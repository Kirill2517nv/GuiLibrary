#include "gui_library.h"
#include "imgui.h"
#include <vector>
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846 
#endif

int widhtWindow = 850;// ширина окна
int hieghtWindow = 1200;// высота окна

float t = 0.0f;    // текущее глобальное время
float dt = 0.05f;  // добавка ко времени
int m = 1; // масса тела в безразмерных единицах
int g = 1; // ускорение свободного падения в безразмерных единицах
int l = 1; // длина стержня в безразмерных единицах

bool pause;

// Начальные парметры 
float alpha = 0.0f; // угол отклонения маятника от вертикали
float v = 1.f; // скорость тела

DataArray buffer(10, 2000, alpha, v); // объект для хранения точек для отрисовки
Scale scale(500, 500, -1.1, 1.1, -1.1, 1.1); // объект для задания шкалы

// функция обработки нажатия на кнопку (Запуск с новой скоростью)
void click_button() {
    t = 0;
    alpha = 0;
    v = get_float_param("Velocity");
    buffer.fill_value(alpha, v);
}

//основная вычислительная функция
void calculation_function() {
    pause = get_bool_param("Pause");

    if (pause)
        return;
    
    if (!pause) 
        t += dt; // шаг времени с учётом скорости
    
    // считаем угол по формуле
    alpha += v / l * dt;

    if (alpha > M_PI) alpha -= 2.0f * M_PI ;
    if (alpha < -M_PI) alpha += 2.0f * M_PI;

// __ для начала рисуем маятник __

    // координаты груза
    float x_m = l * sin(alpha);
    float y_m = -l * cos(alpha);

    // Нарисуем линию от (0,0) до (x, y)
    std::vector<float> mx = {0.0f, x_m};
    std::vector<float> my = {0.0f, y_m};

    // обновляем график маятника
    clear_plot("Pendulum");
    add_plot_line("Pendulum", mx, my, "Pendulum", BLUE, 2.f);
    add_plot_scatter("Pendulum", mx[1], my[1], "Pendulum", RED, 6.f);

// __ рисуем фазовую диаграмму __   
    // считаем скорость по формуле
    v += - g * sin(alpha) * dt;

    // добавляем новую точку (alpha, v) 
    buffer.addPoint(alpha, v);

    // получаем данные для графика
    std::vector<float> x = buffer.getX();
    std::vector<float> y = buffer.getY();

    // обновляем график фазовой диаграммы
    clear_plot("Phase diagram");
    add_plot_scatterline("Phase diagram", x, y, "Phase diagram", BLUE);
    add_plot_scatter("Phase diagram", x[buffer.head], y[buffer.head], "Phase diagram", RED, 6.f);
}

int main() {
    if (!init_gui_library("Task_2: The movement of the pendulum", widhtWindow, hieghtWindow)) return -1;

    add_bool_param("Pause", false);
    add_button_param("Restart", click_button);
    
    add_float_param("Velocity", v);

    create_plot("Pendulum", scale);
    create_plot("Phase diagram", scale);

    set_calculation_function(calculation_function);

    while (gui_main_loop()) {
        sleep_ms(16);
    }

    shutdown_gui_library();
    return 0;
}
