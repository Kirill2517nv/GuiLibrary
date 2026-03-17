#include "gui_library.h"
#include <vector>
#include <cmath>
#include <algorithm>

int widhtWindow = 1200; // ширина окна
int hieghtWindow = 1000; // высота окна

const int Nx = 100;          // ширина решетки
const int Ny = 100;          // высота решетки
const int Q = 9;             // количество скоросте

float tau = 1.0;             // время релаксации

// Дискретные скорости
int Cx[Q] = { 0, 1, 0, -1, 0, 1, -1, -1, 1 };
int Cy[Q] = { 0, 0, 1, 0, -1, 1, 1, -1, -1 };

// Весовые коэффициенты
float Wk[Q] = { 4.0f / 9.0f,               // нулевая скорость
               1.0f / 9.0f, 1.0f / 9.0f,    // скорости по осям
               1.0f / 9.0f, 1.0f / 9.0f,
               1.0f / 36.0f, 1.0f / 36.0f,  // диагональные скорости
               1.0f / 36.0f, 1.0f / 36.0f };

// Массивы распределений
float f[Nx * Ny * Q];           // текущие распределения
std::vector<float> rho(Nx* Ny);             // плотность
std::vector<float> ux(Nx* Ny);              // скорость по x
std::vector<float> uy(Nx* Ny);              // скорость по y

// Характеристика Окна
const int hm_rows = Ny;
const int hm_cols = Nx;
Scale hm_scale(800, 800, 0.f, 100.f, 0.f, 100.f);

int index(int x, int y) {
    return y * Nx + x;
}
int index_q(int x, int y, int q) {
    return Nx * Ny * q + y * Nx + x;
}

// Функция вычисления равновесной функции распределения
float equilibrium(int q, float rho_val, float ux_val, float uy_val) {
    float u2 = ux_val * ux_val + uy_val * uy_val;
    float cu;

    switch (q) {
    case 0: cu = 0; break;
    case 1: cu = ux_val; break;
    case 2: cu = uy_val; break;
    case 3: cu = -ux_val; break;
    case 4: cu = -uy_val; break;
    case 5: cu = ux_val + uy_val; break;
    case 6: cu = -ux_val + uy_val; break;
    case 7: cu = -ux_val - uy_val; break;
    case 8: cu = ux_val - uy_val; break;
    default: cu = 0;
    }

    return Wk[q] * rho_val * (1.0f + 3.0f * cu + 4.5f * cu * cu - 1.5f * u2);
}
// Функция вычисления макроскопических величин
void compute_macroscopic() {
    for (int y = 0; y < Ny; y++) {
        for (int x = 0; x < Nx; x++) {
            int idx = index(x, y);

            // Вычисление плотности
            rho[idx] = 0;
            for (int q = 0; q < Q; q++) {
                rho[idx] += f[index_q(x, y, q)];
            }

            // Вычисление скорости
            float ux_sum = 0, uy_sum = 0;
            for (int q = 0; q < Q; q++) {
                ux_sum += f[index_q(x, y, q)] * Cx[q];
                uy_sum += f[index_q(x, y, q)] * Cy[q];
            }

            if (rho[idx] > 1e-6) {  // избегаем деления на ноль
                ux[idx] = ux_sum / rho[idx];
                uy[idx] = uy_sum / rho[idx];
            }
            else {
                ux[idx] = 0;
                uy[idx] = 0;
            }
        }
    }
}

void calculation_function() {

    clear_plot("Heatmap");
    add_plot_heatmap("Heatmap", rho, hm_rows, hm_cols,
        "rho", 0, 1.5);
    add_plot_heatmap("Heatmap", ux, hm_rows, hm_cols,
        "ux", 0, 1.5);


    for (int x = 0; x < Nx; x++)
    {
        for (int y = 0; y < Ny; y++)
        {
            for (size_t q = 0; q < 9; q++)
            {
                f[index_q(x, y, q)] = f[index_q(x, y, q)] + (equilibrium(q, rho[index(x, y)], ux[index(x, y)], uy[index(x, y)]) - f[index_q(x, y, q)]) / tau;
            }

        }
    }

    compute_macroscopic();


}

int main() {
    //инициализация основного окна
    if (!init_gui_library("LBM", widhtWindow, hieghtWindow)) return -1;
    add_bool_param("Pause", false);


    for (int x = 0; x < Nx / 2; x++)
    {
        for (int y = 0; y < Ny; y++)
        {
            rho[index(x, y)] = 1.1;
            rho[index(x + Nx / 2, y)] = 0.9;

        }
    }

    for (int x = 0; x < Nx; x++)
    {
        for (int y = 0; y < Ny; y++)
        {
            double u = ux[index(x, y)] * ux[index(x, y)] + uy[index(x, y)] * uy[index(x, y)];
            int a = 3;
            float b = 9. / 2.;
            float d = 3. / 2.;


            f[index_q(x, y, 0)] = Wk[0] * rho[index(x, y)] * (1 - d * u);
            f[index_q(x, y, 1)] = Wk[1] * rho[index(x, y)] * (1 + a * ux[index(x, y)] + b * ux[index(x, y)] * ux[index(x, y)] - d * u);
            f[index_q(x, y, 2)] = Wk[2] * rho[index(x, y)] * (1 + a * uy[index(x, y)] + b * uy[index(x, y)] * uy[index(x, y)] - d * u);
            f[index_q(x, y, 3)] = Wk[3] * rho[index(x, y)] * (1 - a * ux[index(x, y)] + b * ux[index(x, y)] * ux[index(x, y)] - d * u);
            f[index_q(x, y, 4)] = Wk[4] * rho[index(x, y)] * (1 - a * uy[index(x, y)] + b * uy[index(x, y)] * uy[index(x, y)] - d * u);
            f[index_q(x, y, 5)] = Wk[5] * rho[index(x, y)] * (1 + a * (ux[index(x, y)] + uy[index(x, y)]) + b * (ux[index(x, y)] + uy[index(x, y)]) * (ux[index(x, y)] + uy[index(x, y)]) - d * u);
            f[index_q(x, y, 6)] = Wk[6] * rho[index(x, y)] * (1 + a * (uy[index(x, y)] - ux[index(x, y)]) + b * (ux[index(x, y)] - uy[index(x, y)]) * (ux[index(x, y)] - uy[index(x, y)]) - d * u);
            f[index_q(x, y, 7)] = Wk[7] * rho[index(x, y)] * (1 - a * (ux[index(x, y)] + uy[index(x, y)]) + b * (ux[index(x, y)] + uy[index(x, y)]) * (ux[index(x, y)] + uy[index(x, y)]) - d * u);
            f[index_q(x, y, 8)] = Wk[8] * rho[index(x, y)] * (1 + a * (ux[index(x, y)] - uy[index(x, y)]) + b * (ux[index(x, y)] - uy[index(x, y)]) * (ux[index(x, y)] - uy[index(x, y)]) - d * u);
        }
    }

    create_plot("Heatmap", hm_scale);

    set_calculation_function(calculation_function);
    //основной цикл работы программы (считай->рисуй)
    while (gui_main_loop()) {
        sleep_ms(16);
    }
    //завершение работы 
    shutdown_gui_library();
    return 0;
}

