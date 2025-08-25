#include "gui_library.h"
#include <iostream>
#include <cmath>
#define _USE_MATH_DEFINES
#include <math.h>

int main() {
    // Инициализация GUI библиотеки
    if (!init_gui_library("Тест Docking и Русского Языка")) {
        std::cerr << "Ошибка инициализации GUI библиотеки" << std::endl;
        return -1;
    }

    // Добавляем параметры для демонстрации
    add_float_param("amplitude", "Амплитуда", 1.0f, 0.1f, 5.0f, 0.1f);
    add_float_param("frequency", "Частота", 1.0f, 0.1f, 10.0f, 0.1f);
    add_float_param("phase", "Фаза", 0.0f, 0.0f, 6.28f, 0.1f);
    add_int_param("points", "Количество точек", 100, 10, 1000, 10);
    add_bool_param("show_sine", "Показать синус", true);
    add_bool_param("show_cosine", "Показать косинус", false);
    add_string_param("title", "Заголовок графика", "График функций");

    // Создаем графики
    create_plot("main_plot", "Основной график");
    create_plot("phase_plot", "Фазовый график");

    // Функция для расчета и обновления графиков
    set_calculation_function([]() {
        float amplitude = get_float_param("amplitude");
        float frequency = get_float_param("frequency");
        float phase = get_float_param("phase");
        int points = get_int_param("points");
        bool show_sine = get_bool_param("show_sine");
        bool show_cosine = get_bool_param("show_cosine");
        std::string title = get_string_param("title");

        // Очищаем графики
        clear_plot("main_plot");
        clear_plot("phase_plot");

        // Создаем массивы данных
        std::vector<float> x_values = create_data_array(points);
        std::vector<float> sine_values = create_data_array(points);
        std::vector<float> cosine_values = create_data_array(points);
        std::vector<float> combined_values = create_data_array(points);

        // Заполняем данные
        for (int i = 0; i < points; i++) {
            float x = (float)i / (points - 1) * 4.0f * M_PI;
            x_values[i] = x;
            
            float sine = amplitude * sin(frequency * x + phase);
            float cosine = amplitude * cos(frequency * x + phase);
            
            sine_values[i] = sine;
            cosine_values[i] = cosine;
            combined_values[i] = sine + cosine;
        }

        // Добавляем данные на графики
        if (show_sine) {
            add_plot_data("main_plot", x_values, sine_values, "Синус");
        }
        if (show_cosine) {
            add_plot_data("main_plot", x_values, cosine_values, "Косинус");
        }
        add_plot_data("main_plot", x_values, combined_values, "Сумма");

        // Фазовый график
        add_plot_data("phase_plot", sine_values, cosine_values, "Фазовая диаграмма");
    });

    std::cout << "Запуск приложения с поддержкой docking и русского языка..." << std::endl;
    std::cout << "Инструкции:" << std::endl;
    std::cout << "- Перетаскивайте окна для их докирования" << std::endl;
    std::cout << "- Используйте правую кнопку мыши на заголовке окна для меню" << std::endl;
    std::cout << "- Изменяйте параметры для обновления графиков" << std::endl;

    // Основной цикл приложения
    while (gui_main_loop()) {
        // Приложение работает
    }

    // Завершение работы
    shutdown_gui_library();
    return 0;
}
