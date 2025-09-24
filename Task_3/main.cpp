#include "gui_library.h"
#include <vector>
#include <cmath>

int widhtWindow = 1200;
int hieghtWindow = 800;

float dt = 0.02f;  // добавка ко времени
float R;  // расстояние от планеты до Солнца
float x_0 = 2.5f, y_0 = 0.0f; // начальные координаты планеты
float x = x_0, y = y_0; // координаты планеты в момент времени t
float v_x = 0.0f, v_y = 0.5f; // скорость планеты по x и y
float a_x, a_y;  // ускорение планеты по x и y
float a;      // модуль ускорения
float alpha = 1.f;

DataBuffer buffer(10, 20000, x, y); // создание объекта
Scale scale(700, 700, -3.f, 3.f, -3.f, 3.f); // создание объекта для задания шкалы

void click_button() {
    v_x = 0.0f;
    v_y = get_float_param("Velocity");
    buffer.fill_value(x_0, y_0);
    x = x_0; y = y_0;
}

void calculation_function(){
    bool pause = get_bool_param("Pause");
    
    if (pause)
        return;

    x += v_x * dt;  
    y += v_y * dt;

    R = sqrt(x * x + y * y);

    a = alpha / (R * R);
    a_x = - x / R * a;
    a_y = - y / R * a;

    v_x += a_x * dt;
    v_y += a_y * dt;
    
    // добавляем новую точку (alpha, v) 
    buffer.addPoint(x, y);

    // получаем данные для графика
    std::vector<float> x = buffer.getX();
    std::vector<float> y = buffer.getY();

    // обновляем график фазовой диаграммы
    clear_plot("Gravity");
    add_plot_scatterline("Gravity", x, y, "Planet", BLUE);
    add_plot_scatter("Gravity", x[buffer.head], y[buffer.head], "Planet", RED, 8.0f);
    add_plot_scatter("Gravity", 0, 0, "Sun", YELLOW, 13.0f);
}

int main() {
    if (!init_gui_library("Task_3: Gravity", widhtWindow, hieghtWindow)) return -1;

    add_bool_param("Pause", false);
    add_button_param("Restart", click_button);
    add_float_param("Velocity", v_y);

    create_plot("Gravity", scale);

    set_calculation_function(calculation_function);

    while (gui_main_loop()) {
        sleep_ms(16);
    }

    shutdown_gui_library();
    return 0;
}
