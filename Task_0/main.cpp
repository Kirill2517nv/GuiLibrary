#include "gui_library.h"
#include <vector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846 
#endif

int widhtWindow = 1200;
int hieghtWindow = 800;

float t = 0.0f;
float dt = 0.01f;  // добавка ко времени
float x_0 = 0.0f, y_0 = 3.0f; // начальные координаты тела
float V = 8.f; // начальная скорость тела
float g = 9.8f;      // модуль ускорения свободного падения 
float alpha = 30.f; // начальный угол наклона

// Нарисуем линию от (x0, y0) до (x1, y1)
std::vector<float> target_x = {8.5, 8.5}; 
std::vector<float> target_y = {3.0, 3.5};

DataArray buffer(100, 20000, x_0, y_0); // создание объекта
Scale scale(700, 700, -0.1f, 10.f, -0.1f, 5.f); // создание объекта для задания шкалы

void click_button() {
    // инициализируем параметры
    alpha = get_float_param("Angle");
    V = get_float_param("Velocity");
    x_0 = get_float_param("x_0");
    y_0 = get_float_param("y_0");

    // обновление значений
    t = 0; 

    // очистка буфера
    buffer.fill_value(x_0, y_0);
}

void calculation_function(){
    bool pause = get_bool_param("Pause");

    
    dt = get_float_param("dt");
    // если pause = true, то программа ставится на паузу
    if (pause)
        return;

    // увеличиваем время на шаг по времени dt
    t += dt;

    // считаем координаты тела по формулам
    float x = x_0 + V * cos(alpha * M_PI / 180.) * t;  
    float y = y_0 + V * sin(alpha * M_PI / 180.) * t - g * t * t / 2.;

    // если тело упало на землю, то ничего не делается 
    if (y <= 0.0f)
        return;

    // добавляем новую точку (x, y) 
    buffer.addPoint(x, y);

    // получаем данные для графика
    std::vector<float> X = buffer.getX();
    std::vector<float> Y = buffer.getY();

    // условие попадания в мишень
    if (((x >= target_x[0] && x <= target_x[0] + V * cos(alpha * M_PI / 180.) * dt) && (y >= target_y[0] && y <= target_y[1]))) {
        set_bool_param("Pause", true);
        dt = 0; t = 0;
    }

    // обновляем график 
    clear_plot("Movement at an angle");

    // создаём тело (add_plot_scatter) и его траекторию (add_plot_scatterline)
    add_plot_scatterline("Movement at an angle", X, Y, "Body", BLUE);
    add_plot_scatter("Movement at an angle", X[buffer.head], Y[buffer.head], "Body", RED, 5.f);

    // создаём мишень: отрисовываем линию (add_plot_line) и добавляем точки на границах (add_plot_scatter)
    add_plot_line("Movement at an angle", target_x, target_y, "Target", WHITE, 2.f);
    add_plot_scatter("Movement at an angle", target_x[0], target_y[0], "Target", WHITE, 3.f);
    add_plot_scatter("Movement at an angle", target_x[1], target_y[1], "Target", WHITE, 3.f);
}

int main() {
    if (!init_gui_library("Task_0: Movement at an angle", widhtWindow, hieghtWindow)) return -1;
    setlocale(LC_ALL, "Russian");

    add_bool_param("Pause", false);
    add_button_param("Restart", click_button);
    add_float_param("Velocity", V);
    add_float_param("Angle", alpha);
    add_float_param("dt", dt);
    add_float_param("x_0", x_0);
    add_float_param("y_0", y_0);

    create_plot("Movement at an angle", scale);

    set_calculation_function(calculation_function);

    while (gui_main_loop()) {
        sleep_ms(16);
    }

    shutdown_gui_library();
    return 0;
}