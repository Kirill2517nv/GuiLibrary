#pragma once
#include "imgui.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <deque>
#include <iostream>

#define RED ImVec4(1.0f, 0.0f, 0.0f, 1.0f)
#define BLUE ImVec4(0.0f, 0.0f, 1.0f, 1.0f)
#define YELLOW ImVec4(1.0f, 1.0f, 0.0f, 1.0f)
#define GREEN ImVec4(0.0f, 1.0f, 0.0f, 1.0f)
#define BLACK ImVec4(0.0f, 0.0f, 0.0f, 1.0f)
#define WHITE ImVec4(1.0f, 1.0f, 1.0f, 1.0f)

// Простой API для школьников - никаких классов, только функции и лямбды

class DataBuffer {
public:
    std::vector<float> x;   // массив X (время в пределах окна)
    std::vector<float> y;   // массив значений функции
    size_t maxSize;         // число точек в буфере
    size_t head;            // текущая позиция для записи
    float window;           // ширина окна по X


    DataBuffer(float windowWidth, size_t points = 200, float x_0 = 0.0f, float y_0 = 0.0f)
        : maxSize(points), head(0), window(windowWidth)
    {
        x.resize(points, x_0);
        y.resize(points, y_0);
    }

    void addPoint(float t, float value) {
        head = (head + 1) % maxSize;
        // отображаем время в окно [0, window)
        float t_window = fmod(t, window);

        x[head] = t_window;
        y[head] = value;
    }

    void clear(float x_value, float y_value) {
        head = 0;

        std::fill(x.begin(), x.end(), x_value);
        std::fill(y.begin(), y.end(), y_value);
    }

    std::vector<float> getX() const { return x; }
    std::vector<float> getY() const { return y; }
};

// Типы параметров
enum class ParamType {
    Float,      // Число с плавающей точкой
    Int,        // Целое число
    Bool,       // Логическое значение
    String,      // Строка
    Button
};

struct Scatter {
    float x_values;
    float y_values;
    std::string label;
    bool visible = true;
    ImVec4 color;
    float size;

    void clear() {
        x_values = 0.0f;
        y_values = 0.0f;
        label.clear();
        visible = true;
        color = ImVec4(0, 0, 0, 0);
        size = 0.0f;
    }
};

struct ScatterLine {
    std::vector<float> x_values;
    std::vector<float> y_values;
    std::string label;
    bool visible = true;
    float size;
    ImVec4 color;

    void clear() {
        x_values.clear();
        y_values.clear();
        label.clear();
        visible = true;
        color = ImVec4(0, 0, 0, 0);
        size = 0.0f;
    }
};

struct FillLine {
    std::vector<float> x_values;
    std::vector<float> y_values;
    std::string label;
    bool visible = true;

    void clear() {
        x_values.clear();
        y_values.clear();
        label.clear();
        visible = true;
    }
};

// Структура для хранения данных масштаба
struct Scale {
    int width;
    int height;
    float x_min;
    float x_max;
    float y_min;
    float y_max;

    Scale(int width = 1200, int height = 800, float x_min = -1.f, float x_max = 1.f, float y_min = -1.f, float y_max = 1.f) : width(width), height(height),
            x_min(x_min), x_max(x_max), y_min(y_min), y_max(y_max) {}
};

// Структура для хранения данных графика
struct PlotData {
    std::vector<Scatter> scatterVector;
    std::vector<ScatterLine> scatterlineVector;
    std::vector<FillLine> lineVector;
    Scale scale;

    PlotData() = default;
    PlotData(const Scale& scale_) : scale(scale_) {}

    void clear() {
        for (auto& s : scatterVector) s.clear();

        for (auto& sl : scatterlineVector) sl.clear();

        for (auto& l : lineVector) l.clear();

    }
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
    std::function<void()> function;
    float min_value = 0.0f;
    float max_value = 100.0f;
    float step = 1.0f;
    bool use_slider = false;
};

// Глобальные функции для работы с GUI

// Инициализация библиотеки
bool init_gui_library(const std::string& window_title = "Численное моделирование", const int widthWindow = 1200, const int heightWindow = 800);

// Основной цикл приложения
bool gui_main_loop();

// Завершение работы
void shutdown_gui_library();

// === ФУНКЦИИ ДЛЯ РАБОТЫ С ПАРАМЕТРАМИ ===

// Добавить параметр типа float
void add_float_param(const std::string& name, 
                    float initial_value = 0.0f, float min = 0.0f, float max = 100.0f, float step = 0.2f, bool use_slider = false);

// Добавить параметр типа int
void add_int_param(const std::string& name, 
                  int initial_value = 0, int min = 0, int max = 100, int step = 1, bool use_slider = false);

// Добавить параметр типа bool
void add_bool_param(const std::string& name, bool initial_value = false);

// Добавить параметр типа string
void add_string_param(const std::string& name, const std::string& initial_value = "");

void add_button_param(const std::string& name, std::function<void()> function);

// Получить значение параметра
float get_float_param(const std::string& name);
int get_int_param(const std::string& name);
bool get_bool_param(const std::string& name);
void set_bool_param(const std::string& name, bool value);
std::string get_string_param(const bool& name);  

// === ФУНКЦИИ ДЛЯ РАБОТЫ С ГРАФИКАМИ ===

// Создать новый график
void create_plot(const std::string& name, const Scale& scale);

// Добавить данные на график
// void add_plot_data(const std::string& plot_name, const std::vector<float>& x, const std::vector<float>& y, const Scale& scale,
//                   const std::string& label = "Данные", const size_t step = 0);

void add_plot_scatter(const std::string& plot_name, const float& x, const float& y, 
                    const std::string& label = "Данные", const ImVec4& color = BLACK, const float& size = 1.0f);

void add_plot_scatterline(const std::string& plot_name, const std::vector<float>& x, const std::vector<float>& y, 
                    const std::string& label = "Данные", const ImVec4& color = BLACK, const float& size = 1.0f);

void add_plot_line(const std::string& plot_name, const std::vector<float>& x, const std::vector<float>& y, 
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
