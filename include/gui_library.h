#pragma once
#include "imgui.h"
#include <string>
#include <vector>
#include <functional>
#include <cmath>
#include <variant>

#define RED    ImVec4(1.0f, 0.0f, 0.0f, 1.0f)
#define BLUE   ImVec4(0.0f, 0.0f, 1.0f, 1.0f)
#define YELLOW ImVec4(1.0f, 1.0f, 0.0f, 1.0f)
#define GREEN  ImVec4(0.0f, 1.0f, 0.0f, 1.0f)
#define BLACK  ImVec4(0.0f, 0.0f, 0.0f, 1.0f)
#define WHITE  ImVec4(1.0f, 1.0f, 1.0f, 1.0f)


// Кольцевой буфер для потоковых данных в реальном времени
class DataArray {
public:
    std::vector<float> x;
    std::vector<float> y;
    size_t maxSize;
    size_t head;
    float window;  // ширина скользящего окна по X (0 = абсолютное время)

    DataArray(float windowWidth, size_t points = 200, float x_0 = 0.0f, float y_0 = 0.0f)
        : maxSize(points), head(0), window(windowWidth)
    {
        x.resize(points, x_0);
        y.resize(points, y_0);
    }

    void addPoint(float t, float value) {
        head = (head + 1) % maxSize;
        x[head] = window == 0.f ? t : fmod(t, window);
        y[head] = value;
    }

    void fill_value(float x_value, float y_value) {
        head = 0;
        std::fill(x.begin(), x.end(), x_value);
        std::fill(y.begin(), y.end(), y_value);
    }

    std::vector<float> getX() const { return x; }
    std::vector<float> getY() const { return y; }
};

// Типы параметров UI
enum class ParamType {
    Float,
    Int,
    Bool,
    String,
    Button
};

// Одиночная точка на графике (размер маркера в пикселях)
struct PlotPoint {
    float x_value = 0.f;
    float y_value = 0.f;
    std::string label;
    bool visible = true;
    ImVec4 color;
    float size = 1.f;
};

// Закрашенный круг с радиусом в координатах графика
struct PlotDisk {
    float x_value = 0.f;
    float y_value = 0.f;
    float radius  = 0.5f;  // в единицах оси X графика
    std::string label;
    bool visible = true;
    ImVec4 color;
};

// Набор точек (scatter-серия)
struct PlotPoints {
    std::vector<float> x_values;
    std::vector<float> y_values;
    std::string label;
    bool visible = true;
    float size = 1.f;
    ImVec4 color;
};

// Сплошная линия
struct PlotLine {
    std::vector<float> x_values;
    std::vector<float> y_values;
    std::string label;
    bool visible = true;
    ImVec4 color;
    float size = 1.f;
};

// История одной потоковой величины. Хранится внутри графика, поэтому коду
// учебной задачи не нужны собственные кольцевые буферы.
struct PlotHistory {
    std::vector<float> x_values;
    std::vector<float> y_values;
    std::string label;
    ImVec4 color;
    float size = 1.f;
    size_t next = 0;
    size_t count = 0;
};

// Тепловая карта
struct Heatmap {
    std::vector<float> values;      // данные row-major (rows * cols)
    int rows = 0;
    int cols = 0;
    double scale_min = 0.0;         // 0 = авто
    double scale_max = 0.0;         // 0 = авто
    std::string label;
    bool visible = true;
    int colormap = 4;               // ImPlotColormap_Viridis
    std::string label_fmt;          // пустая строка = без подписей ячеек
};

// Масштаб осей графика (без размера в пикселях — это не свойство шкалы)
struct Scale {
    float x_min;
    float x_max;
    float y_min;
    float y_max;

    Scale(float x_min = -1.f, float x_max = 1.f,
          float y_min = -1.f, float y_max = 1.f)
        : x_min(x_min), x_max(x_max), y_min(y_min), y_max(y_max) {}
};

// Данные одного графика
struct PlotData {
    std::vector<PlotPoint>  pointVector;
    std::vector<PlotPoints> pointsVector;
    std::vector<PlotLine>   lineVector;
    std::vector<Heatmap>    heatmapVector;
    std::vector<PlotDisk>   diskVector;
    std::vector<PlotHistory> historyVector;
    Scale scale;
    bool scale_dirty = false;
    int width  = 600;
    int height = 400;

    PlotData() = default;
    PlotData(const Scale& scale_, int w = 600, int h = 400)
        : scale(scale_), width(w), height(h) {}

    void clear() {
        pointVector.clear();
        pointsVector.clear();
        lineVector.clear();
        heatmapVector.clear();
        diskVector.clear();
    }
};

// Параметр UI
struct Parameter {
    std::string name;
    std::string label;
    ParamType type;
    std::variant<float, int, bool, std::string> value;
    std::function<void()> function;  // только для Button
    float min_value  = 0.0f;
    float max_value  = 100.0f;
    float step       = 1.0f;
    bool  use_slider = false;
};

// ============================================================
// Инициализация / жизненный цикл
// ============================================================

bool init_gui_library(const std::string& window_title = "Численное моделирование",
                      int widthWindow = 1200, int heightWindow = 800);

bool gui_main_loop();

// Запускает главный цикл. Работает как в нативной, так и в WASM-сборке —
// скрывает #ifdef __EMSCRIPTEN__ от кода задачи.
void run_gui_library();

void shutdown_gui_library();

// ============================================================
// Параметры
// ============================================================

void add_float_param(const std::string& name,
                     float initial_value = 0.0f, float min = 0.0f, float max = 100.0f,
                     float step = 0.2f, bool use_slider = false);

void add_int_param(const std::string& name,
                   int initial_value = 0, int min = 0, int max = 100,
                   int step = 1, bool use_slider = false);

void add_bool_param(const std::string& name, bool initial_value = false);

void add_string_param(const std::string& name, const std::string& initial_value = "");

void add_button_param(const std::string& name, std::function<void()> function);

float       get_float_param(const std::string& name);
void        set_float_param(const std::string& name, float value);
int         get_int_param(const std::string& name);
void        set_int_param(const std::string& name, int value);
bool        get_bool_param(const std::string& name);
void        set_bool_param(const std::string& name, bool value);
std::string get_string_param(const std::string& name);
void        set_string_param(const std::string& name, const std::string& value);

// ============================================================
// Графики
// ============================================================

// Создать новый график. width/height — начальный размер в пикселях (можно менять через слайдеры).
void create_plot(const std::string& name, const Scale& scale,
                 int width = 600, int height = 400);

// Учебная перегрузка: позволяет задать границы без отдельного объекта Scale.
void create_plot(const std::string& name,
                 float x_min, float x_max, float y_min, float y_max,
                 int width = 600, int height = 400);

// Обновить границы осей (применяется на следующем кадре)
void set_plot_scale(const std::string& name,
                    float x_min, float x_max, float y_min, float y_max);

// Нарисовать закрашенный круг с радиусом в координатах графика.
// Если оси X и Y имеют разный масштаб — круг будет выглядеть как эллипс
// (что физически корректно: форма объекта сохраняется в пространстве данных).
void add_plot_disk(const std::string& plot_name, float x, float y, float radius,
                   const std::string& label = "Данные",
                   const ImVec4& color = BLACK);

// Нарисовать одну точку
void add_plot_point(const std::string& plot_name, float x, float y,
                    const std::string& label = "Данные",
                    const ImVec4& color = BLACK, float size = 1.0f);

// Нарисовать набор точек (scatter)
void add_plot_points(const std::string& plot_name,
                     const std::vector<float>& x, const std::vector<float>& y,
                     const std::string& label = "Данные",
                     const ImVec4& color = BLACK, float size = 1.0f);

// Перегрузка для double-данных
void add_plot_points(const std::string& plot_name,
                     const std::vector<double>& x, const std::vector<double>& y,
                     const std::string& label = "Данные",
                     const ImVec4& color = BLACK, float size = 1.0f);

// Нарисовать сплошную линию
void add_plot_line(const std::string& plot_name,
                   const std::vector<float>& x, const std::vector<float>& y,
                   const std::string& label = "Данные",
                   const ImVec4& color = BLUE, float size = 1.0f);

// Добавить точку в историю именованной серии. Библиотека сама хранит не более
// max_points последних значений и рисует их в хронологическом порядке.
void add_plot_history_point(const std::string& plot_name,
                            float x, float y,
                            const std::string& label = "Данные",
                            const ImVec4& color = BLUE, float size = 1.0f,
                            size_t max_points = 2000);

// Очистить всю накопленную историю графика (например, по кнопке Restart).
void clear_plot_history(const std::string& plot_name);

// Очистить только одну именованную серию.
void clear_plot_history(const std::string& plot_name, const std::string& label);

// Перегрузка для double-данных
void add_plot_line(const std::string& plot_name,
                   const std::vector<double>& x, const std::vector<double>& y,
                   const std::string& label = "Данные",
                   const ImVec4& color = BLUE, float size = 1.0f);

void add_plot_heatmap(const std::string& plot_name,
                      const std::vector<float>& values,
                      int rows, int cols,
                      const std::string& label    = "Heatmap",
                      double scale_min = 0.0, double scale_max = 0.0,
                      int colormap = 4,
                      const std::string& label_fmt = "");

// Очистить все серии графика (вызывать каждый кадр перед добавлением новых данных)
void clear_plot(const std::string& plot_name);

// ============================================================
// Расчёт
// ============================================================

// Установить функцию, вызываемую каждый кадр перед рендерингом UI
void set_calculation_function(std::function<void()> calc_func);

// ============================================================
// Разметка окон (docking layout)
// ============================================================

// Автоматически расположить окна: Parameters слева (params_ratio от ширины),
// все графики справа в виде вкладок.
// В WASM: применяется при каждом запуске (браузер не сохраняет позиции).
// В нативной версии: применяется только при первом запуске, пока нет imgui.ini.
void set_auto_layout(float params_ratio = 0.25f);

// Расширенный вариант: полный контроль над разметкой.
// Callback получает ID корневого dockspace; внутри него используй layout_split_* и layout_dock().
// Пример:
//   set_default_layout([](ImGuiID id) {
//       ImGuiID left, right;
//       layout_split_left(id, 0.3f, &left, &right);
//       layout_dock("Parameters", left);
//       layout_dock("График",     right);
//   });
void set_default_layout(std::function<void(ImGuiID dockspace_id)> layout_fn);

// ── Вспомогательные функции для использования внутри set_default_layout ──

// Разбить узел вертикально: левая часть получает ratio*100% ширины
void layout_split_left(ImGuiID node, float ratio, ImGuiID* out_left, ImGuiID* out_right);

// Разбить узел горизонтально: верхняя часть получает ratio*100% высоты
void layout_split_up(ImGuiID node, float ratio, ImGuiID* out_top, ImGuiID* out_bottom);

// Поместить окно с именем name в узел node
void layout_dock(const std::string& name, ImGuiID node);

// ============================================================
// Утилиты
// ============================================================

void   sleep_ms(int milliseconds);
double get_time();
