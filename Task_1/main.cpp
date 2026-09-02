#include "gui_library.h"
#include <cmath>

float t = 0.0f;    // текущее глобальное время
float dt = 0.15f;  // шаг по времени
// функция обработки нажатия на кнопку (очистка графика)
void click_clear() {
    t = 0;
    set_float_param("Time", t);
    clear_plot_history("Sin");
}

//основная вычислительная функция
void calculation_function(){
    bool pause = get_bool_param("Pause");
    if (pause) return;

    t += dt;
    set_float_param("Time", t);

    float A1 = get_float_param("Amplitude 1");
    float w1 = get_float_param("Frequency 1");
    float A2 = get_float_param("Amplitude 2");
    float w2 = get_float_param("Frequency 2");

    float y_t = A1 * sin(w1 * t) + A2 * sin(w2 * t);

    clear_plot("Sin");
    add_plot_history_point("Sin", t, y_t,
                           "y = A1*sin(omega1*t) + A2*sin(omega2*t)",
                           BLUE, 1.0f, 20000);
    add_plot_point("Sin", t, y_t, "Current value", RED, 5.0f);
}

int main() {
    if (!init_gui_library("Task_1: The movement of the sin")) return -1;

    add_button_param("Restart", click_clear);
    add_float_param("Amplitude 1", 1.0f);
    add_float_param("Frequency 1", 1.0f);
    add_float_param("Amplitude 2", 1.0f);
    add_float_param("Frequency 2", 1.0f);
    add_float_param("Time", t);
    add_bool_param("Pause", false);

    create_plot("Sin", 0.f, 200.f, -2.f, 2.f, 750, 475);

    set_calculation_function(calculation_function);

    run_gui_library();
    return 0;
}
