#include "gui_library.h"
#include <vector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846 
#endif

int widhtWindow = 1200; // ширина окна
int hieghtWindow = 800; // высота окна

float t = 0.0f; // время
float dt = 0.01f;  // шаг по времени
float x_0 = 0.0f, y_0 = 0.0f; // начальные координаты тела
float V = 8.f; // начальная скорость тела
float g = 9.8f;      // модуль ускорения свободного падения 
float alpha = 45.f; // начальный угол наклона


std::vector<float> target_x = {8., 8.}; // X координаты мишени
std::vector<float> target_y = {2.0, 3.}; // Y координаты мишени

// объект для хранения точек для отрисовки
DataArray buffer(100, 2000, x_0, y_0); //аргументы (ширина зацикливания, кол-во точек, начальные координаты)
// создание объекта для задания шкалы
Scale scale(800, 700, -0.1f, 10.f, -0.1f, 5.f); //аргументы (ширина графика, высота графика, минимальное значение X, максимальное значение X, минимальное значение Y, максимальное значение Y)

// функция обработки нажатия на кнопку
void click_button() {
    // инициализируем параметры
    alpha = get_float_param("Angle");
    V = get_float_param("Velocity");
    x_0 = get_float_param("x_0");
    y_0 = get_float_param("y_0");

    // обновление значений
    t = 0; 

    // очистка буфера (заполнение значением по умолчанию)
    buffer.fill_value(x_0, y_0);
}

//основная вычислительная функция
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
    std::vector<float> X = buffer.getX(); // получить массив X координат
    std::vector<float> Y = buffer.getY(); // получить массив Y координат

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
    //инициализация основного окна
    if (!init_gui_library("Task_0: Movement at an angle", widhtWindow, hieghtWindow)) return -1;
    //Добавить ЧекБокс
    add_bool_param("Pause", false); 
    //Добавить кнопку
    add_button_param("Restart", click_button);
    //Добавить параметр типа float
    add_float_param("Velocity", V);
    add_float_param("Angle", alpha);
    add_float_param("dt", dt);
    add_float_param("x_0", x_0);
    add_float_param("y_0", y_0);

    create_plot("Movement at an angle", scale);

    //установить расчетную функцию
    set_calculation_function(calculation_function);

    //основной цикл работы программы (считай->рисуй)
    while (gui_main_loop()) {
        sleep_ms(16);
    }
    //завершение работы 
    shutdown_gui_library();
    return 0;
}