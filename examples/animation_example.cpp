#include "gui_library.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main() {
    // Инициализация библиотеки
    if (!init_gui_library("Пример: Анимированная волна")) {
        return -1;
    }

    // Добавляем параметры
    add_float_param("amplitude", "Амплитуда", 2.0f, 0.1f, 5.0f, 0.1f);
    add_float_param("wavelength", "Длина волны", 1.0f, 0.1f, 5.0f, 0.1f);
    add_float_param("speed", "Скорость", 1.0f, 0.1f, 5.0f, 0.1f);
    add_bool_param("pause", "Пауза", false);
    add_int_param("points", "Точки", 200, 50, 500, 10);

    // Создаем график
    create_plot("Волна", "Бегущая волна");

    // Переменная для времени
    double start_time = get_time();

    // Функция расчета с анимацией
    auto calculation_function = [start_time]() {
        // Получаем параметры
        float amplitude = get_float_param("amplitude");
        float wavelength = get_float_param("wavelength");
        float speed = get_float_param("speed");
        bool pause = get_bool_param("pause");
        int points = get_int_param("points");

        // Вычисляем текущее время
        double current_time = get_time() - start_time;
        if (pause) current_time = 0.0;

        // Создаем массивы для данных
        std::vector<float> x_values(points);
        std::vector<float> y_values(points);

        // Заполняем данные волны
        for (int i = 0; i < points; ++i) {
            float x = (float)i / (points - 1) * 10.0f; // От 0 до 10
            x_values[i] = x;
            
            // Формула бегущей волны: y = A * sin(2π * (x/λ - t/T))
            float k = 2.0f * M_PI / wavelength; // Волновое число
            float omega = speed * k; // Угловая частота
            y_values[i] = amplitude * sin(k * x - omega * current_time);
        }

        // Обновляем график
        clear_plot("Волна");
        add_plot_data("Волна", x_values, y_values, "Волна");
    };

    // Устанавливаем функцию расчета
    set_calculation_function(calculation_function);

    // Основной цикл
    while (gui_main_loop()) {
        // Небольшая задержка для плавной анимации
        sleep_ms(16); // ~60 FPS
    }

    // Завершение работы
    shutdown_gui_library();
    return 0;
}
