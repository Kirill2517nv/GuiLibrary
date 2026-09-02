#include "gui_library.h"
#include <cmath>

int widhtWindow = 1200;
int hieghtWindow = 800;

float dt = 0.02f;
float R;
float x_0 = 2.5f, y_0 = 0.0f;
float x = x_0, y = y_0;
float v_x = 0.0f, v_y = 0.5f;
float a_x, a_y;
float a;
float alpha = 1.f;

void click_button() {
    v_x = 0.0f;
    v_y = get_float_param("Velocity");
    x = x_0; y = y_0;
    clear_plot_history("Gravity");
    add_plot_history_point("Gravity", x, y, "Planet trajectory",
                           BLUE, 1.f, 20000);
}

void calculation_function(){
    bool pause = get_bool_param("Pause");
    if (pause) return;

    x += v_x * dt;
    y += v_y * dt;

    R = sqrt(x * x + y * y);

    a = alpha / (R * R);
    a_x = -x / R * a;
    a_y = -y / R * a;

    v_x += a_x * dt;
    v_y += a_y * dt;

    clear_plot("Gravity");
    add_plot_history_point("Gravity", x, y, "Planet trajectory",
                           BLUE, 1.f, 20000);
    add_plot_point("Gravity", x, y, "Planet", RED, 8.0f);
    add_plot_point("Gravity", 0, 0, "Sun", YELLOW, 13.0f);
}

int main() {
    if (!init_gui_library("Task_3: Gravity", widhtWindow, hieghtWindow)) return -1;

    add_bool_param("Pause", false);
    add_button_param("Restart", click_button);
    add_float_param("Velocity", v_y);

    create_plot("Gravity", -3.f, 3.f, -3.f, 3.f, 700, 700);

    // Начальное положение планеты до первого шага моделирования.
    add_plot_history_point("Gravity", x, y, "Planet trajectory",
                           BLUE, 1.f, 20000);

    set_calculation_function(calculation_function);
    set_auto_layout();

    run_gui_library();
    return 0;
}
