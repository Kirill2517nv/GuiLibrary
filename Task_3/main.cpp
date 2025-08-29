#include "gui_library.h"
#include <vector>
#include <cmath>

float dt = 0.02f;  // добавка ко времени
float R;  // расстояние от планеты до Солнца
float x = 2.5, y = 0.0; // координаты планеты
float v_x = 0.0, v_y = 0.5; // скорость планеты по x и y
float a_x, a_y;  // ускорение планеты по x и y
float a;      // модуль ускорения
float alpha = 1.f;

DataBuffer buffer(10, 20000, x, y); // создание объекта

void calculation_function(){
    bool pause = get_bool_param("pause");
    
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
    clear_plot("Движение планеты");
    add_plot_data("Движение планеты", x, y, "Планета", buffer.head);
}

int main() {
    if (!init_gui_library("Task_3: Движение планеты")) return -1;


    add_bool_param("pause", "Пауза", false);

    create_plot("Движение планеты", "Движение планеты");

    set_calculation_function(calculation_function);

    while (gui_main_loop()) {
        sleep_ms(16);
    }

    shutdown_gui_library();
    return 0;
}
