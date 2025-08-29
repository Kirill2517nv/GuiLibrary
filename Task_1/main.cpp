#include "gui_library.h"
#include <vector>
#include <cmath>


float t = 0.0f;    // текущее глобальное время
float dt = 0.15f;  // добавка ко времени
DataBuffer buffer(30, 200); // создание объекта

void calculation_function(){
    bool pause = get_bool_param("pause");
        if (pause) return;
        if (!pause) 
            t += dt; // шаг времени с учётом скорости
        
        // инициализируем параметры
        float A = get_float_param("amplitude1");
        float w = get_float_param("frequency1");

        // считаем y = A * sin(ωt)
        float y_t = A * sin(w * t);

        // добавляем новую точку (t, A*sin(ωt))
        buffer.addPoint(t, y_t);

        // получаем данные для графика
        std::vector<float> x = buffer.getX();
        std::vector<float> y = buffer.getY();

        // обновляем график
        clear_plot("Sin");
        add_plot_data("Sin", x, y, "y = A*sin(ωt)");
}

int main() {
    if (!init_gui_library("Task_1: Двигающаяся синусоида")) return -1;

    add_float_param("amplitude1", "Амплитуда 1", 1.0f);
    add_float_param("frequency1", "Частота 1", 1.0f);
    add_float_param("amplitude2", "Амплитуда 2", 1.0f);
    add_float_param("frequency2", "Частота 2", 1.0f);
    add_float_param("amplitude3", "Амплитуда 3", 1.0f);
    add_float_param("frequency3", "Частота 3", 1.0f);

    add_bool_param("pause", "Пауза", false);

    create_plot("Sin", "y = A * sin(ωt)");

    set_calculation_function(calculation_function);

    while (gui_main_loop()) {
        sleep_ms(16);
    }

    shutdown_gui_library();
    return 0;
}
