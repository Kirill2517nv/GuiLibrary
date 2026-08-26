#include "gui_library.h"
#include <vector>
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int widhtWindow = 850;
int hieghtWindow = 1200;

float t = 0.0f;    // текущее глобальное время
float dt = 0.05f;  // шаг по времени
int m = 1;
int g = 1;
int l = 1;

bool pause;

float alpha = 0.0f;  // угол отклонения маятника
float v = 1.f;       // скорость тела

DataArray buffer(10, 2000, alpha, v);
Scale scale(-1.1f, 1.1f, -1.1f, 1.1f);

void click_button() {
    t = 0;
    alpha = 0;
    v = get_float_param("Velocity");
    buffer.fill_value(alpha, v);
}

void calculation_function() {
    pause = get_bool_param("Pause");
    if (pause) return;

    t += dt;
    set_float_param("Time", t);

    alpha += v / l * dt;
    if (alpha >  M_PI) alpha -= 2.0f * (float)M_PI;
    if (alpha < -M_PI) alpha += 2.0f * (float)M_PI;

    // координаты груза
    float x_m = l * sin(alpha);
    float y_m = -l * cos(alpha);

    std::vector<float> mx = {0.0f, x_m};
    std::vector<float> my = {0.0f, y_m};

    clear_plot("Pendulum");
    add_plot_line("Pendulum", mx, my, "Pendulum", BLUE, 2.f);
    add_plot_point("Pendulum", mx[1], my[1], "Pendulum", RED, 6.f);

    v += -g * sin(alpha) * dt;

    buffer.addPoint(alpha, v);

    std::vector<float> x = buffer.getX();
    std::vector<float> y = buffer.getY();

    clear_plot("Phase diagram");
    add_plot_points("Phase diagram", x, y, "Phase diagram", BLUE);
    add_plot_point("Phase diagram", x[buffer.head], y[buffer.head], "Phase diagram", RED, 6.f);
}

int main() {
    if (!init_gui_library("Task_2: The movement of the pendulum", widhtWindow, hieghtWindow)) return -1;

    add_bool_param("Pause", false);
    add_button_param("Restart", click_button);
    add_float_param("Velocity", v);
    add_float_param("Time", t);

    create_plot("Pendulum",      scale, 500, 500);
    create_plot("Phase diagram", scale, 500, 500);

    set_calculation_function(calculation_function);

    run_gui_library();
    return 0;
}
