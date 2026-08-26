#include "gui_library.h"
#include <vector>
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

DataArray buffer(10, 20000, x, y);
Scale scale(-3.f, 3.f, -3.f, 3.f);

void click_button() {
    v_x = 0.0f;
    v_y = get_float_param("Velocity");
    buffer.fill_value(x_0, y_0);
    x = x_0; y = y_0;
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

    buffer.addPoint(x, y);

    std::vector<float> bx = buffer.getX();
    std::vector<float> by = buffer.getY();

    clear_plot("Gravity");
    add_plot_points("Gravity", bx, by, "Planet", BLUE);
    add_plot_point("Gravity", bx[buffer.head], by[buffer.head], "Planet", RED, 8.0f);
    add_plot_point("Gravity", 0, 0, "Sun", YELLOW, 13.0f);
}

int main() {
    if (!init_gui_library("Task_3: Gravity", widhtWindow, hieghtWindow)) return -1;

    add_bool_param("Pause", false);
    add_button_param("Restart", click_button);
    add_float_param("Velocity", v_y);

    create_plot("Gravity", scale, 700, 700);

    set_calculation_function(calculation_function);
    set_auto_layout();

    run_gui_library();
    return 0;
}
