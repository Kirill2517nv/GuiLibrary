#include "gui_library.h"
#include <vector>
#include <cmath>

float t = 0.0f;    // текущее глобальное время
float dt = 0.05f;  // добавка ко времени
DataBuffer buffer(30, 20000); // создание объекта
Scale scale(750, 475, 0., 30., -2., 2.);

void click_clear() {
    t = 0;
    buffer.fill_value(0.f, 0.f);
}

void calculation_function(){
    bool pause = get_bool_param("pause");
        if (pause) return;
        if (!pause) 
            t += dt; // шаг времени с учётом скорости
        
        // инициализируем параметры
        float A1 = get_float_param("Амплитуда 1");
        float w1 = get_float_param("Частота 1");

        float A2 = get_float_param("Амплитуда 2");
        float w2 = get_float_param("Частота 2");

        // считаем y = A * sin(ωt)
        float y_t = A1 * sin(w1 * t) + A2 * sin(w2 * t);

        // добавляем новую точку (t, A*sin(ωt))
        buffer.addPoint(t, y_t);

        // получаем данные для графика
        std::vector<float> x = buffer.getX();
        std::vector<float> y = buffer.getY();

        // обновляем график
        clear_plot("Sin");
        add_plot_scatterline("Sin", x, y, "y = A1*sin(ω1*t) + A2*sin(ω2*t)", BLUE);
        add_plot_scatter("Sin", x[buffer.head], y[buffer.head], "y = A1*sin(ω1*t) + A2*sin(ω2*t)", RED);
}

int main() {
    if (!init_gui_library("Task_1: Двигающаяся синусоида")) return -1;

    add_button_param("Очистка", click_clear);
    add_float_param("Амплитуда 1", 1.0f);
    add_float_param("Частота 1", 1.0f);
    add_float_param("Амплитуда 2", 1.0f);
    add_float_param("Частота 2", 1.0f);
// сделать кнопку с очисткой буфера
    add_bool_param("Пауза", false);

    create_plot("Sin", scale);

    set_calculation_function(calculation_function);

    while (gui_main_loop()) {
        sleep_ms(16);
    }

    shutdown_gui_library();
    return 0;
}
