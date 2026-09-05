// ============================================================
// Заготовка для своей задачи.
//
// Как сделать своё задание:
//   1. скопируйте папку Template под своим именем, например MyTask;
//   2. в корневом CMakeLists.txt добавьте две строки по образцу остальных:
//          option(BUILD_MYTASK "Собирать MyTask" ON)
//          if(BUILD_MYTASK)
//              add_subdirectory(MyTask)
//          endif()
//   3. в MyTask/CMakeLists.txt замените Template на MyTask;
//   4. соберите:  bash scripts/bootstrap.sh MyTask
//
// Здесь показан минимум: одна величина, которая меняется со временем, один
// график и кнопка «Заново». Разбор того, как это устроено, – в разделе
// «Основы C++» на сайте курса.
// ============================================================

#include "gui_library.h"

// Состояние задачи. Объявлено снаружи функций, потому что должно копиться от
// кадра к кадру: переменная внутри функции рождается и умирает на каждом кадре.
float t = 0.0f;   // время
float y = 1.0f;   // величина, за которой следим

// Кнопка «Заново»: вернуть всё в начальное состояние.
void restart() {
    t = 0.0f;
    y = 1.0f;
    clear_plot_history("График");
}

// Один шаг расчёта. Библиотека зовёт эту функцию каждый кадр – сама она
// ничего не повторяет.
void calculation_function() {
    if (get_bool_param("Пауза")) return;

    float dt = get_float_param("Шаг по времени");
    float k  = get_float_param("Коэффициент k");

    // Здесь ваша физика. Пример: экспоненциальное затухание dy/dt = -k*y
    y += -k * y * dt;
    t += dt;

    set_float_param("Время", t);
    set_float_param("Значение", y);

    add_plot_history_point("График", t, y, "y(t)", BLUE, 1.f, 20000);
}

int main() {
    if (!init_gui_library("Моя задача")) return -1;

    add_bool_param("Пауза", false);
    add_button_param("Заново", restart);
    add_float_param("Коэффициент k", 0.5f, 0.f, 5.f, 0.05f);
    add_float_param("Шаг по времени", 0.02f, 0.001f, 0.5f, 0.001f);
    add_output_float("Время", 0.f);
    add_output_float("Значение", 0.f);

    create_plot("График", 0.f, 10.f, 0.f, 1.2f, 750, 500);
    set_plot_axes("График", "t, с", "y");

    set_calculation_function(calculation_function);
    set_auto_layout();

    run_gui_library();
    return 0;
}
