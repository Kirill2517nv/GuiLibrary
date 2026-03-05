#include "gui_library.h"
#include <cmath>
#include <vector>
#include <cstdlib>
#include <iostream>

// Параметры области
const int nx = 100;
const int ny = 100;
const double rc = 3.0;

// Частица
struct Particle {
    double ar = 5.0;
    double b = 2.5, e = 2.0;
    double x, y;
    double vx, vy, v2;
    double ax = 0, ay = 0, ax1 = 0, ay1 = 0;
};

// Глобальные переменные симуляции
std::vector<Particle> prtcls;
int n_prtcls = 100;
double t = 0;
double dt = 0.002;
int iteration = 0;
double E_kin = 0, U = 0;

// Буферы для графиков энергии
DataArray da_ekin(0.f, 2000, 0.f, 0.f);
DataArray da_u(0.f, 2000, 0.f, 0.f);
DataArray da_etotal(0.f, 2000, 0.f, 0.f);

// Инициализация частиц
void init_particles() {
    prtcls.resize(n_prtcls);
    t = 0;
    iteration = 0;

    for (int i = 0; i < n_prtcls; i++) {
        prtcls[i].b = get_float_param("b0");
        prtcls[i].x = 0.1 * nx + 1.7 * prtcls[i].b * (i % 20);
        prtcls[i].y = 0.1 * ny + 1.7 * prtcls[i].b * (i / 20);
        srand(i);
        prtcls[i].vx = 4.1 * (rand() % 200 - 100) / 100.0;
        prtcls[i].vy = 4.1 * (rand() % 200 - 100) / 100.0;
        prtcls[i].v2 = prtcls[i].vx * prtcls[i].vx + prtcls[i].vy * prtcls[i].vy;
        prtcls[i].ax = 0; prtcls[i].ay = 0;
        prtcls[i].ax1 = 0; prtcls[i].ay1 = 0;
    }

    // Начальные условия как в оригинале
    if (n_prtcls >= 2) {
        prtcls[0].vx = 0; prtcls[0].vy = -1.2;
        prtcls[1].vx = 0; prtcls[1].vy = 1.2;
        prtcls[0].x = 0.3 * nx; prtcls[0].y = 0.1 * ny;
        prtcls[1].x = 0.3 * nx; prtcls[1].y = 0.9 * ny;
    }

    // Сброс буферов энергии
    da_ekin.fill_value(0.f, 0.f);
    da_u.fill_value(0.f, 0.f);
    da_etotal.fill_value(0.f, 0.f);
}

// Обновление позиций (Velocity Verlet, шаг 1)
void update_positions() {
    for (int i = 0; i < n_prtcls; i++) {
        double dx_val = prtcls[i].vx * dt + 0.5 * prtcls[i].ax * dt * dt;
        double dy_val = prtcls[i].vy * dt + 0.5 * prtcls[i].ay * dt * dt;
        prtcls[i].x += dx_val;
        prtcls[i].y += dy_val;
        // Периодические граничные условия
        if (prtcls[i].x >= nx) prtcls[i].x -= nx;
        if (prtcls[i].x < 0) prtcls[i].x += nx;
        if (prtcls[i].y >= ny) prtcls[i].y -= ny;
        if (prtcls[i].y < 0) prtcls[i].y += ny;
    }
}

// Минимальное расстояние с учётом периодических ГУ
void min_image(double xi, double yi, double xj, double yj, double& dx, double& dy) {
    double dx0 = std::abs(xi - xj);
    double dy0 = std::abs(yi - yj);

    if (nx - dx0 < dx0) {
        dx = xj - xi - nx;
    } else {
        dx = xj - xi;
    }
    if (ny - dy0 < dy0) {
        dy = yj - yi - ny;
    } else {
        dy = yj - yi;
    }
    if (xj < xi && nx - dx0 < dx0) {
        dx += 2 * nx;
    }
    if (yj < yi && ny - dy0 < dy0) {
        dy += 2 * ny;
    }
}

// Вычисление ускорений (потенциал Леннарда-Джонса)
void compute_forces() {
    for (int i = 0; i < n_prtcls; i++) {
        prtcls[i].ax1 = 0;
        prtcls[i].ay1 = 0;
    }
    for (int i = 0; i < n_prtcls - 1; i++) {
        for (int j = i + 1; j < n_prtcls; j++) {
            double dx, dy;
            min_image(prtcls[i].x, prtcls[i].y, prtcls[j].x, prtcls[j].y, dx, dy);
            double d = sqrt(dx * dx + dy * dy);
            if (d < rc * prtcls[i].b && d > 0.01) {
                double p6 = pow(prtcls[i].b / d, 6);
                double aij = 12.0 * prtcls[i].e * (p6 - p6 * p6) / d / prtcls[i].ar;
                double aijx = aij * dx / d;
                double aijy = aij * dy / d;
                prtcls[i].ax1 += aijx;
                prtcls[i].ay1 += aijy;
                prtcls[j].ax1 -= aijx;
                prtcls[j].ay1 -= aijy;
            }
        }
    }
}

// Обновление скоростей (Velocity Verlet, шаг 2)
void update_velocities() {
    for (int i = 0; i < n_prtcls; i++) {
        prtcls[i].vx += 0.5 * dt * (prtcls[i].ax + prtcls[i].ax1);
        prtcls[i].vy += 0.5 * dt * (prtcls[i].ay + prtcls[i].ay1);
        prtcls[i].v2 = prtcls[i].vx * prtcls[i].vx + prtcls[i].vy * prtcls[i].vy;
        prtcls[i].ax = prtcls[i].ax1;
        prtcls[i].ay = prtcls[i].ay1;
    }
}

// Вычисление кинетической энергии
void compute_ekin() {
    E_kin = 0;
    for (int i = 0; i < n_prtcls; i++) {
        E_kin += prtcls[i].ar * prtcls[i].v2 / 2.0;
    }
}

// Вычисление потенциальной энергии
void compute_u() {
    U = 0;
    for (int i = 0; i < n_prtcls - 1; i++) {
        for (int j = i + 1; j < n_prtcls; j++) {
            double dx, dy;
            min_image(prtcls[i].x, prtcls[i].y, prtcls[j].x, prtcls[j].y, dx, dy);
            double d = sqrt(dx * dx + dy * dy);
            if (d < rc * prtcls[i].b && d != 0) {
                double bd6 = pow(prtcls[i].b / d, 6);
                U += prtcls[i].e * (bd6 * bd6 - 2 * bd6);
            }
        }
    }
}

// Функция расчёта, вызываемая каждый кадр
void calculation_function() {
    if (get_bool_param("Pause")) return;

    dt = get_float_param("dt");
    n_prtcls = static_cast<int>(prtcls.size());

    // Velocity Verlet
    update_positions();
    compute_forces();
    update_velocities();

    t += dt;
    iteration++;

    // Вычисление энергий
    compute_ekin();
    compute_u();

    // Обновление графиков энергии
    float tf = static_cast<float>(t);
    da_ekin.addPoint(tf, static_cast<float>(E_kin));
    da_u.addPoint(tf, static_cast<float>(U));
    da_etotal.addPoint(tf, static_cast<float>(E_kin + U));

    clear_plot("Energy");
    add_plot_line("Energy", da_ekin.x, da_ekin.y, "E_kin", RED, 2.0f);
    add_plot_line("Energy", da_u.x, da_u.y, "U", YELLOW, 2.0f);
    add_plot_line("Energy", da_etotal.x, da_etotal.y, "E_total", WHITE, 2.0f);

    // Обновление графика частиц
    clear_plot("Particles");
    for (int i = 0; i < n_prtcls; i++) {
        add_plot_scatter("Particles",
            static_cast<float>(prtcls[i].x),
            static_cast<float>(prtcls[i].y),
            "p" + std::to_string(i),
            WHITE,
            static_cast<float>(prtcls[i].b));
    }

    // Обновление отображаемых параметров
    set_float_param("Time", static_cast<float>(t));
    set_float_param("E_kin", static_cast<float>(E_kin));
    set_float_param("U", static_cast<float>(U));
    set_float_param("E_total", static_cast<float>(E_kin + U));
}

int main() {
    if (!init_gui_library("Molecular Dynamics", 1400, 900)) return -1;

    // Параметры UI
    add_float_param("dt", 0.002f, 0.0001f, 0.01f, 0.0005f);
    add_float_param("b0", 2.5f, 0.5f, 5.0f, 0.1f);
    add_bool_param("Pause", false);
    add_button_param("Restart", []() { init_particles(); });
    add_float_param("Time", 0.0f, 0.0f, 1e6f, 0.0f);
    add_float_param("E_kin", 0.0f, -1e6f, 1e6f, 0.0f);
    add_float_param("U", 0.0f, -1e6f, 1e6f, 0.0f);
    add_float_param("E_total", 0.0f, -1e6f, 1e6f, 0.0f);

    // Графики
    create_plot("Particles", Scale(600, 600, 0.f, static_cast<float>(nx), 0.f, static_cast<float>(ny)));
    create_plot("Energy", Scale(600, 400, 0.f, 10.f, -100.f, 100.f));

    // Инициализация частиц
    init_particles();

    set_calculation_function(calculation_function);

    while (gui_main_loop()) {
        sleep_ms(16);
    }

    shutdown_gui_library();
    return 0;
}
