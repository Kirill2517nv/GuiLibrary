#include "gui_library.h"
#include <vector>
#include <cmath>
#include <algorithm>

// ============================================================
// Параметры окна
// ============================================================

const int windowWidth  = 1200;
const int windowHeight = 1000;

// ============================================================
// Параметры решётки D2Q9
// ============================================================

int Nx = 100;
int Ny = 100;
const int Q  = 9;

// Дискретные скорости D2Q9
const int Cx[Q] = { 0,  1,  0, -1,  0,  1, -1, -1,  1 };
const int Cy[Q] = { 0,  0,  1,  0, -1,  1,  1, -1, -1 };

// Весовые коэффициенты
const float Wk[Q] = {
    4.0f / 9.0f,
    1.0f / 9.0f,  1.0f / 9.0f,  1.0f / 9.0f,  1.0f / 9.0f,
    1.0f / 36.0f, 1.0f / 36.0f, 1.0f / 36.0f, 1.0f / 36.0f
};

// Противоположные направления (для bounce-back)
const int opposite[Q] = { 0, 3, 4, 1, 2, 7, 8, 5, 6 };

// ============================================================
// Глобальные массивы
// ============================================================

std::vector<bool>  solid;   // маска твёрдых узлов
std::vector<float> f;       // функции распределения
std::vector<float> f_new;   // буфер для стриминга

std::vector<float> rho;         // плотность
std::vector<float> ux;          // скорость x (решёточная, без коррекции силы)
std::vector<float> uy;          // скорость y
std::vector<float> vel_mag;     // |u| физическая (для визуализации)
std::vector<float> vel_x_vis;   // ux физическая (для визуализации)
std::vector<float> vel_y_vis;   // uy физическая (для визуализации)

// Выделение памяти при изменении размера сетки
void resize_grid() {
    solid.assign(Nx * Ny, false);
    f.assign(Nx * Ny * Q, 0.f);
    f_new.assign(Nx * Ny * Q, 0.f);
    rho.assign(Nx * Ny, 1.f);
    ux.assign(Nx * Ny, 0.f);
    uy.assign(Nx * Ny, 0.f);
    vel_mag.assign(Nx * Ny, 0.f);
    vel_x_vis.assign(Nx * Ny, 0.f);
    vel_y_vis.assign(Nx * Ny, 0.f);
}

// Накопленные данные K(φ)
std::vector<float> phi_data;
std::vector<float> K_data;

// Флаги кнопок
bool needs_restart = false;
bool needs_record  = false;

// Счётчик итераций
int iteration = 0;

// ============================================================
// Вспомогательные функции
// ============================================================

int idx(int x, int y)          { return y * Nx + x; }
int idx_q(int x, int y, int q) { return Nx * Ny * q + y * Nx + x; }

// Равновесная функция распределения f_eq(q, rho, ux, uy)
float equilibrium(int q, float r, float u, float v) {
    float cu = Cx[q] * u + Cy[q] * v;
    float u2 = u * u + v * v;
    return Wk[q] * r * (1.0f + 3.0f * cu + 4.5f * cu * cu - 1.5f * u2);
}

// ============================================================
// Настройка препятствий — массив цилиндров
// ============================================================

void setup_obstacles() {
    int r   = get_int_param("Cylinder radius");
    int ncx = get_int_param("Cylinders X");
    int ncy = get_int_param("Cylinders Y");

    std::fill(solid.begin(), solid.end(), false);
    if (r <= 0) return;

    float spacing_x = (float)Nx / ncx;
    float spacing_y = (float)Ny / ncy;

    for (int i = 0; i < ncx; i++) {
        for (int j = 0; j < ncy; j++) {
            // Шахматный порядок: нечётные ряды смещены на полшага по X
            float offset_x = (j % 2 == 1) ? spacing_x * 0.5f : 0.0f;
            float cx = spacing_x * (i + 0.5f) + offset_x;
            float cy = spacing_y * (j + 0.5f);

            for (int y = 0; y < Ny; y++) {
                for (int x = 0; x < Nx; x++) {
                    float dx = x - cx;
                    float dy = y - cy;
                    if (dx * dx + dy * dy < (float)(r * r))
                        solid[idx(x, y)] = true;
                }
            }
        }
    }
}

// ============================================================
// Инициализация симуляции
// ============================================================

void init_simulation() {
    Nx = get_int_param("Nx");
    Ny = get_int_param("Ny");
    resize_grid();
    setup_obstacles();
    iteration = 0;

    for (int y = 0; y < Ny; y++) {
        for (int x = 0; x < Nx; x++) {
            int i  = idx(x, y);
            rho[i] = 1.0f;
            ux[i]  = 0.0f;
            uy[i]  = 0.0f;
            vel_mag[i] = 0.0f;

            for (int q = 0; q < Q; q++)
                f[idx_q(x, y, q)] = Wk[q]; // f_eq(rho=1, u=0)
        }
    }
}

// ============================================================
// Вычисление макроскопических величин
// (решёточная скорость — без поправки на силу)
// ============================================================

void compute_macroscopic() {
    for (int y = 0; y < Ny; y++) {
        for (int x = 0; x < Nx; x++) {
            int i = idx(x, y);
            if (solid[i]) {
                rho[i] = 0.0f;
                ux[i]  = 0.0f;
                uy[i]  = 0.0f;
                continue;
            }
            float r = 0, u = 0, v = 0;
            for (int q = 0; q < Q; q++) {
                float fq = f[idx_q(x, y, q)];
                r += fq;
                u += fq * Cx[q];
                v += fq * Cy[q];
            }
            rho[i] = r;
            ux[i]  = (r > 1e-6f) ? u / r : 0.0f;
            uy[i]  = (r > 1e-6f) ? v / r : 0.0f;
        }
    }
}

// ============================================================
// Коллизия BGK + объёмная сила методом точной разности (EDM)
// ============================================================

void collision(float tau_val, float G) {
    for (int y = 0; y < Ny; y++) {
        for (int x = 0; x < Nx; x++) {
            if (solid[idx(x, y)]) continue;

            int   i = idx(x, y);
            float r = rho[i];
            float u = ux[i];
            float v = uy[i];

            for (int q = 0; q < Q; q++) {
                int iq = idx_q(x, y, q);
                float feq = equilibrium(q, r, u, v);

                // BGK-коллизия
                f[iq] -= (f[iq] - feq) / tau_val;

                // Метод точной разности (EDM): Δf = f_eq(ρ, u+G) − f_eq(ρ, u)
                f[iq] += equilibrium(q, r, u + G, v) - feq;
            }
        }
    }
}

// ============================================================
// Стриминг с bounce-back
//   X — периодические ГУ
//   Y — стенки (half-way bounce-back)
//   Твёрдые узлы — bounce-back
// ============================================================

void streaming() {
    std::fill(f_new.begin(), f_new.end(), 0.f);

    for (int y = 0; y < Ny; y++) {
        for (int x = 0; x < Nx; x++) {
            if (solid[idx(x, y)]) continue;

            for (int q = 0; q < Q; q++) {
                int nx = (x + Cx[q] + Nx) % Nx; // периодика по X
                int ny = y + Cy[q];

                if (ny >= 0 && ny < Ny && !solid[idx(nx, ny)]) {
                    // Свободный узел — нормальный стриминг
                    f_new[idx_q(nx, ny, q)] = f[idx_q(x, y, q)];
                } else {
                    // Стенка или твёрдый узел — bounce-back
                    f_new[idx_q(x, y, opposite[q])] = f[idx_q(x, y, q)];
                }
            }
        }
    }

    f = f_new;
}

// ============================================================
// Вычисление пористости и проницаемости
// ============================================================

float compute_porosity() {
    int fluid = 0;
    for (int i = 0; i < Nx * Ny; i++)
        if (!solid[i]) fluid++;
    return (float)fluid / (Nx * Ny);
}

// Проницаемость по закону Дарси: K = ν · <u_superficial> / G
// Поверхностная (superficial) скорость — усреднение по ВСЕЙ области,
// включая твёрдые узлы (u=0), как требует закон Дарси.
float compute_permeability(float tau_val, float G) {
    if (G < 1e-10f) return 0.0f;

    float nu = (tau_val - 0.5f) / 3.0f;
    float ux_sum = 0.0f;

    for (int i = 0; i < Nx * Ny; i++) {
        if (!solid[i])
            ux_sum += ux[i] + G * 0.5f; // физическая скорость флюидных узлов
    }

    // Поверхностная скорость = сумма по флюидным / общее число узлов
    float mean_ux_superficial = ux_sum / (Nx * Ny);
    return nu * mean_ux_superficial / G;
}


// ============================================================
// Основная расчётная функция (вызывается каждый кадр)
// ============================================================

void calculation_function() {
    // Читаем параметры UI
    float tau_val = get_float_param("tau");
    float G       = get_float_param("Force x1e-5") * 1e-5f;
    bool  paused  = get_bool_param("Pause");
    int   steps   = get_int_param("Steps/frame");

    // Перезапуск (читает Nx, Ny из UI и пересоздаёт сетку)
    if (needs_restart) {
        init_simulation();
        // Обновляем масштаб тепловых карт под новый размер сетки
        set_plot_scale("Velocity field", 0.f, (float)Nx, 0.f, (float)Ny);
        set_plot_scale("Density",        0.f, (float)Nx, 0.f, (float)Ny);
        {
            float G_val  = get_float_param("Force x1e-5") * 1e-5f;
            float tau_v  = get_float_param("tau");
            float nu_v   = (tau_v - 0.5f) / 3.0f;
            float u_max  = G_val * Ny * Ny / (8.0f * nu_v);
            set_plot_scale("Poiseuille", 0.f, (float)Ny,
                           0.f, std::max(u_max * 1.2f, 1e-6f));
        }
        needs_restart = false;
    }

    // LBM шаги
    if (!paused) {
        for (int s = 0; s < steps; s++) {
            collision(tau_val, G);
            streaming();
            compute_macroscopic();
            iteration++;
        }
    }

    // Физическая скорость для визуализации
    for (int i = 0; i < Nx * Ny; i++) {
        if (!solid[i]) {
            float u_phys = ux[i] + G * 0.5f;
            vel_mag[i]   = std::sqrt(u_phys * u_phys + uy[i] * uy[i]);
            vel_x_vis[i] = u_phys;
            vel_y_vis[i] = uy[i];
        } else {
            vel_mag[i]   = 0.0f;
            vel_x_vis[i] = 0.0f;
            vel_y_vis[i] = 0.0f;
        }
    }

    // Обновляем измерения в UI
    float phi = compute_porosity();
    float K   = compute_permeability(tau_val, G);
    float nu  = (tau_val - 0.5f) / 3.0f;

    set_float_param("Porosity", phi);
    set_float_param("Permeability", K);
    set_int_param("Iteration", iteration);

    // Запись точки K(φ)
    if (needs_record) {
        phi_data.push_back(phi);
        K_data.push_back(K);
        needs_record = false;
    }

    // ==========================================================
    // Отрисовка графиков
    // ==========================================================

    float max_vel = *std::max_element(vel_mag.begin(), vel_mag.end());
    float max_vx  = *std::max_element(vel_x_vis.begin(), vel_x_vis.end());
    float min_vx  = *std::min_element(vel_x_vis.begin(), vel_x_vis.end());
    float max_vy  = *std::max_element(vel_y_vis.begin(), vel_y_vis.end());
    float min_vy  = *std::min_element(vel_y_vis.begin(), vel_y_vis.end());
    float abs_vx  = std::max(std::abs(min_vx), std::abs(max_vx));
    float abs_vy  = std::max(std::abs(min_vy), std::abs(max_vy));

    // 1. Три тепловые карты скорости на одном полотне
    clear_plot("Velocity field");
    add_plot_heatmap("Velocity field", vel_mag,   Ny, Nx,
                     "||u||", 0.0, std::max(max_vel, 1e-6f));
    add_plot_heatmap("Velocity field", vel_x_vis, Ny, Nx,
                     "ux", -std::max(abs_vx, 1e-6f), std::max(abs_vx, 1e-6f));
    add_plot_heatmap("Velocity field", vel_y_vis, Ny, Nx,
                     "uy", -std::max(abs_vy, 1e-6f), std::max(abs_vy, 1e-6f));

    // 2. Тепловая карта плотности
    clear_plot("Density");
    add_plot_heatmap("Density", rho, Ny, Nx,
                     "rho", 0.99, 1.01);

    // 3. Верификация Пуазейля (профиль ux(y) в центре канала)
    clear_plot("Poiseuille");
    {
        int x_mid = Nx / 2;
        std::vector<float> y_coords(Ny), ux_profile(Ny), ux_analytical(Ny);

        for (int y = 0; y < Ny; y++) {
            y_coords[y]  = (float)y;
            ux_profile[y] = ux[idx(x_mid, y)] + G * 0.5f; // физическая скорость

            // Аналитика Пуазейля (half-way bounce-back: стенки на y=-0.5 и y=Ny-0.5)
            ux_analytical[y] = G / (2.0f * nu)
                             * (y + 0.5f) * (Ny - 0.5f - y);
        }

        add_plot_points("Poiseuille", y_coords, ux_profile,
                        "LBM", BLUE, 2.0f);
        add_plot_line("Poiseuille", y_coords, ux_analytical,
                      "Analytical", RED, 2.0f);

        // Масштаб оси Y задаётся только при рестарте — колёсиком можно зумить
    }

    // 4. График K(φ): аналитика Козени–Карман + LBM
    clear_plot("K(phi)");
    {
        const int N_pts = 200;
        std::vector<float> phi_kc(N_pts), K_kc(N_pts);

        float kc_A = get_float_param("KC: A");


        for (int i = 0; i < N_pts; i++) {
            float phi = 0.05f + 0.90f * i / (N_pts - 1); // φ от 0.05 до 0.95
            float c = 1.0f - phi;

            phi_kc[i] = phi;
            // Козени–Карман: K = A · φ^3 / (1 - φ)^2.
            // Было φ^2/(1 - φ): и степень пористости, и степень знаменателя
            // не те. Кривая, с которой ученик сравнивает свои точки LBM, шла
            // не так, и расхождение списывалось на численный метод.
            K_kc[i]   = kc_A * phi * phi * phi / (c * c);
        }

        add_plot_line("K(phi)", phi_kc, K_kc,
                      "Kozeny-Carman", RED, 2.0f);
    }

    // Точки из численного эксперимента
    if (!phi_data.empty()) {
        add_plot_points("K(phi)", phi_data, K_data,
                        "LBM", BLUE, 5.0f);
    }
}

// ============================================================
// main
// ============================================================

int main() {
    if (!init_gui_library("Task_4: LBM, течение в пористой среде", windowWidth, windowHeight))
        return -1;

    // --- Параметры управления ---
#ifdef __EMSCRIPTEN__
    add_int_param("Nx",                 100, 20, 400, 10, false);
    add_int_param("Ny",                 40, 20, 400, 10, false);
#else
    add_int_param("Nx",                100, 20, 400, 10, false);
    add_int_param("Ny",                100, 20, 400, 10, false);
#endif
    add_float_param("tau",             1.0f, 0.55f, 2.0f, 0.05f, true);
    add_float_param("Force x1e-5",     1.0f, 0.1f,  10.0f, 0.1f, true);
    add_int_param("Cylinder radius",   0, 0, 18, 1, true);
    add_int_param("Cylinders X",       3, 1, 6, 1, false);
    add_int_param("Cylinders Y",       3, 1, 4, 1, false);
    add_int_param("Steps/frame",       100, 1, 500, 10, true);
    add_bool_param("Pause", false);

    add_button_param("Restart", []() { needs_restart = true; });
    add_button_param("Record K(phi)", []() { needs_record = true; });

    // --- Подгоночные параметры Козени–Кармана: K = A · φ^2 / (1−φ)^2 ---
    add_float_param("KC: A",  1.0f, 0.0f, 100.0f, 0.1f, false);


    // --- Индикаторы (перезаписываются каждый кадр) ---
    add_float_param("Porosity",          1.0f, 0.0f, 1.0f,    0.01f, false);
    add_float_param("Permeability", 0.0f, 0.0f, 10000.0f, 0.01f, false);
    add_int_param("Iteration",           0,    0,    10000000, 1, false);

    // --- Графики (масштаб обновляется при Restart под новый Nx/Ny) ---
    Scale hm_scale(0.f, (float)Nx, 0.f, (float)Ny);
    create_plot("Velocity field", hm_scale, 500, 400);  // 3 heatmap'а на одном полотне
    create_plot("Density",        hm_scale, 500, 400);

    Scale pois_scale(0.f, 40.f, 0.f, 0.01f);
    create_plot("Poiseuille", pois_scale, 500, 400);

    Scale kphi_scale(0.5f, 1.0f, 0.f, 20.0f);
    create_plot("K(phi)", kphi_scale, 500, 400);

    // --- Разметка (соответствует imgui.ini, применяется всегда в WASM) ---
    set_default_layout([](ImGuiID id) {
        // Схема: Parameters | (Velocity/Density) | (K(phi)/Poiseuille)
        // Пропорции из imgui.ini: 426 | 686 | 1444 по X, 579 | 770 по Y
        ImGuiID params, rest;
        layout_split_left(id, 426.f / 2560.f, &params, &rest);

        ImGuiID left_col, right_col;
        layout_split_left(rest, 850.f / (2560.f - 426.f), &left_col, &right_col);

        ImGuiID vel_node, density_node;
        layout_split_up(left_col, 700.f / 1351.f, &vel_node, &density_node);

        ImGuiID kphi_node, pois_node;
        layout_split_up(right_col, 700.f / 1351.f, &kphi_node, &pois_node);

        layout_dock("Parameters",   params);
        layout_dock("Velocity field", vel_node);
        layout_dock("Density",      density_node);
        layout_dock("K(phi)",       kphi_node);
        layout_dock("Poiseuille",   pois_node);
    });

    // --- Запуск ---
    init_simulation();
    set_calculation_function(calculation_function);
    run_gui_library();
    return 0;
}
