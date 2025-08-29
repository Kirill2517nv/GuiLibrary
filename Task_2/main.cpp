#include "gui_library.h"
#include <vector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846 
#endif

float t = 0.0f;    // текущее глобальное время
float dt = 0.01f;  // добавка ко времени
int m = 1; // масса тела в безразмерных единицах
int g = 1; // ускорение свободного падения в безразмерных единицах
int l = 1; // длина стержня в безразмерных единицах

// Начальные парметры 
float alpha = 0.0f; // угол отклонения маятника от вертикали
float v = 1.f; // скорость тела

DataBuffer buffer(10, 2000, alpha, v); // создание объекта

void calculation_function(){
    bool pause = get_bool_param("pause");
    
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
    clear_plot("Маятник");
    add_plot_data("Маятник", mx, my, "отрезок");

// __ рисуем фазовую диаграмму __   

    // считаем скорость по формуле
    v += - g * sin(alpha) * dt;

    // добавляем новую точку (alpha, v) 
    buffer.addPoint(alpha, v);

    // получаем данные для графика
    std::vector<float> x = buffer.getX();
    std::vector<float> y = buffer.getY();

    // обновляем график фазовой диаграммы
    clear_plot("Фазовая диаграмма");
    add_plot_data("Фазовая диаграмма", x, y, "Фазовая диаграмма", buffer.head);
}

int main() {
    if (!init_gui_library("Task_2: Движение маятника")) return -1;


    add_bool_param("pause", "Пауза", false);

    create_plot("Маятник", "Маятник");
    create_plot("Фазовая диаграмма", "Фазовая диаграмма");

    set_calculation_function(calculation_function);

    while (gui_main_loop()) {
        sleep_ms(16);
    }

    shutdown_gui_library();
    return 0;
}
