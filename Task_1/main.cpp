// ============================================================
// Задача 1. Сложение колебаний.
//
// Складываем два гармонических колебания:
//
//     y(t) = A1 * sin(w1 * t) + A2 * sin(w2 * t)
//
// При близких, но не равных частотах видны биения – медленная огибающая,
// на слух это пульсация громкости.
//
// Подписи здесь по-русски: библиотека рисует кириллицу с версии, где шрифт
// вшит в неё (src/roboto_medium_font.cpp), а исходник читается как UTF-8 на
// всех компиляторах (флаг /utf-8 для MSVC в корневом CMakeLists).
// ============================================================

#include "gui_library.h"
#include <cmath>

float t = 0.0f;    // текущее время

// Обработчик кнопки: начать счёт заново
void click_clear() {
    t = 0;
    set_float_param("Время", t);
    clear_plot_history("Колебания");
}

// Основная вычислительная функция: вызывается каждый кадр
void calculation_function() {
    if (get_bool_param("Пауза")) return;

    // Шаг по времени берём из пульта: задание «поставьте частоту 21, не меняя
    // шага» держится именно на том, что шаг можно потом изменить и увидеть,
    // как две пересекающиеся синусоиды превращаются в нормальный график.
    t += get_float_param("Шаг по времени");
    set_float_param("Время", t);

    float A1 = get_float_param("Амплитуда 1");
    float w1 = get_float_param("Частота 1");
    float A2 = get_float_param("Амплитуда 2");
    float w2 = get_float_param("Частота 2");

    float y_t = A1 * sin(w1 * t) + A2 * sin(w2 * t);

    clear_plot("Колебания");
    add_plot_history_point("Колебания", t, y_t,
                           "y = A1*sin(w1*t) + A2*sin(w2*t)",
                           BLUE, 1.0f, 20000);
    add_plot_point("Колебания", t, y_t, "Текущее значение", RED, 5.0f);
}

int main() {
    if (!init_gui_library("Задача 1. Сложение колебаний")) return -1;

    add_button_param("Заново", click_clear);
    add_float_param("Амплитуда 1", 1.0f);
    add_float_param("Частота 1", 1.0f);
    add_float_param("Амплитуда 2", 1.0f);
    add_float_param("Частота 2", 1.0f);
    add_float_param("Шаг по времени", 0.15f, 0.001f, 1.0f, 0.01f);
    add_output_float("Время", t);   // показание, а не ручка: его пишет расчёт
    add_bool_param("Пауза", false);

    create_plot("Колебания", 0.f, 200.f, -2.f, 2.f, 750, 475);
    set_plot_axes("Колебания", "t, с", "y, м");

    set_calculation_function(calculation_function);

    run_gui_library();
    return 0;
}
