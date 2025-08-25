#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

// Простой API для школьников - никаких классов, только функции и лямбды

// Типы параметров
enum class ParamType {
    Float,      // Число с плавающей точкой
    Int,        // Целое число
    Bool,       // Логическое значение
    String      // Строка
};

// Структура для хранения данных графика
struct PlotData {
    std::vector<float> x_values;
    std::vector<float> y_values;
    std::string label;
    bool visible = true;
};

// Структура для параметра
struct Parameter {
    std::string name;
    std::string label;
    ParamType type;
    union {
        float float_value;
        int int_value;
        bool bool_value;
    };
    std::string string_value;
    float min_value = 0.0f;
    float max_value = 100.0f;
    float step = 1.0f;
};

// Глобальные функции для работы с GUI

// Инициализация библиотеки
bool init_gui_library(const std::string& window_title = "Численное моделирование");

// Основной цикл приложения
bool gui_main_loop();

// Завершение работы
void shutdown_gui_library();

// === ФУНКЦИИ ДЛЯ РАБОТЫ С ПАРАМЕТРАМИ ===

// Добавить параметр типа float
void add_float_param(const std::string& name, const std::string& label, 
                    float initial_value = 0.0f, float min = 0.0f, float max = 100.0f, float step = 1.0f);

// Добавить параметр типа int
void add_int_param(const std::string& name, const std::string& label, 
                  int initial_value = 0, int min = 0, int max = 100, int step = 1);

// Добавить параметр типа bool
void add_bool_param(const std::string& name, const std::string& label, bool initial_value = false);

// Добавить параметр типа string
void add_string_param(const std::string& name, const std::string& label, const std::string& initial_value = "");

// Получить значение параметра
float get_float_param(const std::string& name);
int get_int_param(const std::string& name);
bool get_bool_param(const std::string& name);
std::string get_string_param(const std::string& name);

// === ФУНКЦИИ ДЛЯ РАБОТЫ С ГРАФИКАМИ ===

// Создать новый график
void create_plot(const std::string& name, const std::string& title = "");

// Добавить данные на график
void add_plot_data(const std::string& plot_name, const std::vector<float>& x, const std::vector<float>& y, 
                  const std::string& label = "Данные");

// Очистить график
void clear_plot(const std::string& plot_name);

// === ФУНКЦИИ ДЛЯ РАСЧЕТОВ ===

// Установить функцию расчета (лямбда-функция)
void set_calculation_function(std::function<void()> calc_func);

// Функция для получения размера массива данных графика
int get_plot_data_size(const std::string& plot_name);

// Функция для заполнения данных графика (для школьников)
void fill_plot_data(const std::string& plot_name, int index, float x, float y);

// === УТИЛИТЫ ===

// Создать массив для заполнения данными
std::vector<float> create_data_array(int size);

// Функция для паузы (для анимации)
void sleep_ms(int milliseconds);

// Функция для получения времени
double get_time();
