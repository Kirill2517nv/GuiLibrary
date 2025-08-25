#include "gui_library.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main() {
    // Инициализация библиотеки
    if (!init_gui_library("Пример: Синусоида")) {
        return -1;
    }

    // Добавляем параметры для настройки
    add_float_param("amplitude", "Амплитуда", 1.0f, 0.1f, 5.0f, 0.1f);
    add_float_param("frequency", "Частота", 1.0f, 0.1f, 10.0f, 0.1f);
    add_float_param("phase", "Фаза", 0.0f, 0.0f, 6.28f, 0.1f);
    add_bool_param("show_grid", "Показать сетку", true);
    add_int_param("points", "Количество точек", 100, 10, 1000, 10);

    // Создаем график
    create_plot("График", "Синусоида");

    // Функция расчета (лямбда-функция)
    auto calculation_function = []() {
        // Получаем параметры
        float amplitude = get_float_param("amplitude");
        float frequency = get_float_param("frequency");
        float phase = get_float_param("phase");
        int points = get_int_param("points");

        // Создаем массивы для данных
        std::vector<float> x_values(points);
        std::vector<float> y_values(points);

        // Заполняем данные
        for (int i = 0; i < points; ++i) {
            float x = (float)i / (points - 1) * 4.0f * M_PI; // От 0 до 4π
            x_values[i] = x;
            y_values[i] = amplitude * sin(frequency * x + phase);
        }

        // Очищаем старые данные и добавляем новые
        clear_plot("График");
        add_plot_data("График", x_values, y_values, "sin(x)");
    };

    // Устанавливаем функцию расчета
    set_calculation_function(calculation_function);

    // Основной цикл
    while (gui_main_loop()) {
        // Ничего не делаем здесь - все в функции расчета
    }

    // Завершение работы
    shutdown_gui_library();
    return 0;
}
