#include "gui_library.h"
#include "gui_library_layout.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "implot_internal.h"
#include <GLFW/glfw3.h>
#include <map>
#include <algorithm>
#include <chrono>
#include <iterator>
#include <thread>
#include <iostream>
#include <fstream>
#include <variant>
#include <filesystem>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>

EM_JS(int, get_browser_width,  (), { return window.innerWidth;  });
EM_JS(int, get_browser_height, (), { return window.innerHeight; });

// Прямой обработчик scroll-событий браузера для ImGui.
// GLFW-порт Emscripten (-sUSE_GLFW=3) ненадёжно передаёт scroll в приложение,
// поэтому регистрируем собственный callback через html5.h API.
static EM_BOOL emscripten_wheel_cb(int /*eventType*/, const EmscriptenWheelEvent* we, void* /*userData*/) {
    float dy = -(float)we->deltaY;
    if (we->deltaMode == DOM_DELTA_LINE)  dy *= 15.0f;  // строки → пиксели (приближение)
    if (we->deltaMode == DOM_DELTA_PAGE)  dy *= 400.0f; // страницы → пиксели
    ImGui::GetIO().AddMouseWheelEvent(0.0f, dy / 100.0f);
    return EM_TRUE; // EM_TRUE — браузер не выполняет действие по умолчанию (скролл страницы)
}
#endif


// Вшитый шрифт с кириллицей: определён в src/roboto_medium_font.cpp.
// Объявление здесь, а не в заголовке: массив нужен ровно одной функции, и в
// публичном API библиотеки ему делать нечего.
extern const char RobotoMedium_compressed_data_base85[];

// ============================================================
// Внутренние структуры
// ============================================================
//
// Раньше они стояли в публичном заголовке, и ученик, открывший «документацию»,
// первым делом видел реализацию: PlotPoint, Heatmap, PlotData, а также мёртвый
// класс DataArray, который нигде не использовался. Теперь всё это здесь – в
// единственном файле, которому оно нужно.

enum class ParamType {
    Float,
    Int,
    Bool,
    String,
    Button,
    // Показания: рисуются рамкой только для чтения. Отдельные типы, потому что
    // «Время», «Пористость», «Момент импульса» – это выводы расчёта, а не
    // ручки. Показанные через add_float_param, они выглядели полем ввода:
    // ученик правил значение, а следующий кадр молча его перезатирал.
    OutputFloat,
    OutputInt
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
    float radius  = 0.5f;
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

// История одной потоковой величины: кольцевой буфер внутри графика, поэтому
// коду учебной задачи собственные буферы не нужны.
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
    std::vector<float> values;      // данные построчно (rows * cols)
    int rows = 0;
    int cols = 0;
    double scale_min = 0.0;         // 0 = авто
    double scale_max = 0.0;         // 0 = авто
    std::string label;
    bool visible = true;
    int colormap = 4;               // ImPlotColormap_Viridis
    std::string label_fmt;          // пустая строка = без подписей ячеек
};

// Границы осей графика
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
    std::vector<PlotPoint>   pointVector;
    std::vector<PlotPoints>  pointsVector;
    std::vector<PlotLine>    lineVector;
    std::vector<Heatmap>     heatmapVector;
    std::vector<PlotDisk>    diskVector;
    std::vector<PlotHistory> historyVector;
    Scale scale;
    bool scale_dirty = false;
    int width  = 600;
    int height = 400;
    // Подписи осей вместе с единицами: «t, с», «x, м». Пустые – ось без подписи.
    std::string x_label;
    std::string y_label;

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

// Параметр пульта
struct Parameter {
    std::string name;
    std::string label;
    ParamType type;
    std::variant<float, int, bool, std::string> value;
    std::function<void()> function;  // только для кнопки
    float min_value  = 0.0f;
    float max_value  = 100.0f;
    float step       = 1.0f;
    bool  use_slider = false;
};

// ============================================================
// Оформление
// ============================================================
//
// Цвета взяты из тёмной темы сайта (templates/base.html, static/css/
// design-tokens.css), чтобы симуляция в рамке на странице не выглядела чужой
// программой, случайно вставленной в урок:
//
//     фон страницы   #0f172a   slate-900
//     карточка       #1e293b   slate-800
//     рамка          #334155   slate-700
//     текст          #e2e8f0   slate-200, приглушённый #94a3b8 slate-400
//     акцент         #2563eb   brand-600, светлее #3b82f6 brand-500
//
// Скругления и отступы – от готовых наборов Dear ImGui (github.com/
// GraphicsProgramming/dear-imgui-styles), но умеренные: пульт с параметрами
// плотный, и модные 11-пиксельные скругления съедают место, которого на
// проекторе и так мало.
static ImVec4 rgb(int r, int g, int b, float a = 1.0f) {
    return ImVec4(r / 255.f, g / 255.f, b / 255.f, a);
}

static void apply_site_style() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding    = 8.f;
    style.ChildRounding     = 8.f;
    style.FrameRounding     = 6.f;
    style.PopupRounding     = 8.f;
    style.ScrollbarRounding = 8.f;
    style.GrabRounding      = 6.f;
    style.TabRounding       = 6.f;

    style.WindowBorderSize = 1.f;
    style.FrameBorderSize  = 1.f;
    style.PopupBorderSize  = 1.f;

    style.WindowPadding    = ImVec2(12.f, 10.f);
    style.FramePadding     = ImVec2(10.f, 6.f);
    style.ItemSpacing      = ImVec2(8.f, 8.f);
    style.ItemInnerSpacing = ImVec2(6.f, 6.f);
    style.ScrollbarSize    = 12.f;
    style.GrabMinSize      = 12.f;

    ImVec4* c = style.Colors;
    c[ImGuiCol_Text]                 = rgb(226, 232, 240);
    c[ImGuiCol_TextDisabled]         = rgb(100, 116, 139);
    c[ImGuiCol_WindowBg]             = rgb(30, 41, 59);
    c[ImGuiCol_ChildBg]              = rgb(30, 41, 59);
    c[ImGuiCol_PopupBg]              = rgb(30, 41, 59);
    c[ImGuiCol_Border]               = rgb(51, 65, 85);
    c[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);

    c[ImGuiCol_FrameBg]              = rgb(15, 23, 42);
    c[ImGuiCol_FrameBgHovered]       = rgb(51, 65, 85);
    c[ImGuiCol_FrameBgActive]        = rgb(51, 65, 85);

    c[ImGuiCol_TitleBg]              = rgb(15, 23, 42);
    c[ImGuiCol_TitleBgActive]        = rgb(15, 23, 42);
    c[ImGuiCol_TitleBgCollapsed]     = rgb(15, 23, 42, 0.75f);
    c[ImGuiCol_MenuBarBg]            = rgb(15, 23, 42);

    c[ImGuiCol_ScrollbarBg]          = rgb(15, 23, 42, 0.f);
    c[ImGuiCol_ScrollbarGrab]        = rgb(51, 65, 85);
    c[ImGuiCol_ScrollbarGrabHovered] = rgb(71, 85, 105);
    c[ImGuiCol_ScrollbarGrabActive]  = rgb(100, 116, 139);

    c[ImGuiCol_CheckMark]            = rgb(96, 165, 250);
    c[ImGuiCol_SliderGrab]           = rgb(59, 130, 246);
    c[ImGuiCol_SliderGrabActive]     = rgb(96, 165, 250);

    c[ImGuiCol_Button]               = rgb(37, 99, 235);
    c[ImGuiCol_ButtonHovered]        = rgb(59, 130, 246);
    c[ImGuiCol_ButtonActive]         = rgb(29, 78, 216);

    c[ImGuiCol_Header]               = rgb(37, 99, 235, 0.55f);
    c[ImGuiCol_HeaderHovered]        = rgb(59, 130, 246, 0.70f);
    c[ImGuiCol_HeaderActive]         = rgb(29, 78, 216);

    c[ImGuiCol_Separator]            = rgb(51, 65, 85);
    c[ImGuiCol_SeparatorHovered]     = rgb(59, 130, 246);
    c[ImGuiCol_SeparatorActive]      = rgb(96, 165, 250);

    c[ImGuiCol_ResizeGrip]           = rgb(51, 65, 85, 0.6f);
    c[ImGuiCol_ResizeGripHovered]    = rgb(59, 130, 246, 0.8f);
    c[ImGuiCol_ResizeGripActive]     = rgb(96, 165, 250);

    // Вкладки: графики стоят вкладками рядом друг с другом, и активная должна
    // читаться с проектора через весь кабинет – отсюда подчёркивание акцентом.
    c[ImGuiCol_Tab]                  = rgb(15, 23, 42);
    c[ImGuiCol_TabHovered]           = rgb(51, 65, 85);
    c[ImGuiCol_TabSelected]          = rgb(30, 41, 59);
    c[ImGuiCol_TabSelectedOverline]  = rgb(59, 130, 246);
    c[ImGuiCol_TabDimmed]            = rgb(15, 23, 42);
    c[ImGuiCol_TabDimmedSelected]    = rgb(30, 41, 59);

    c[ImGuiCol_DockingPreview]       = rgb(37, 99, 235, 0.5f);
    c[ImGuiCol_DockingEmptyBg]       = rgb(15, 23, 42);

    c[ImGuiCol_TableHeaderBg]        = rgb(15, 23, 42);
    c[ImGuiCol_TableBorderStrong]    = rgb(51, 65, 85);
    c[ImGuiCol_TableBorderLight]     = rgb(51, 65, 85, 0.5f);
    c[ImGuiCol_TableRowBg]           = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_TableRowBgAlt]        = rgb(255, 255, 255, 0.03f);

    // ImPlot: область графика темнее панели, как холст на карточке. Сетка
    // приглушена намеренно – линии данных должны быть заметнее разметки.
    ImPlotStyle& plot = ImPlot::GetStyle();
    plot.PlotBorderSize = 1.f;
    plot.LineWeight     = 2.f;   // те же 2 пикселя, что у линий на графиках сайта

    ImVec4* pc = plot.Colors;
    pc[ImPlotCol_FrameBg]      = ImVec4(0, 0, 0, 0);
    pc[ImPlotCol_PlotBg]       = rgb(15, 23, 42);
    pc[ImPlotCol_PlotBorder]   = rgb(51, 65, 85);
    pc[ImPlotCol_LegendBg]     = rgb(30, 41, 59, 0.94f);
    pc[ImPlotCol_LegendBorder] = rgb(51, 65, 85);
    pc[ImPlotCol_LegendText]   = rgb(203, 213, 225);
    pc[ImPlotCol_AxisText]     = rgb(148, 163, 184);
    pc[ImPlotCol_AxisGrid]     = rgb(51, 65, 85, 0.55f);
    pc[ImPlotCol_AxisBg]       = ImVec4(0, 0, 0, 0);
    pc[ImPlotCol_Crosshairs]   = rgb(148, 163, 184);
}

// Глобальные переменные
static GLFWwindow* g_window = nullptr;
static std::map<std::string, Parameter> g_parameters;
static std::vector<std::string> g_DrawOrder;
static std::map<std::string, PlotData> g_plots;
static std::function<void()> g_calculation_function = nullptr;
static bool g_should_close = false;
static std::chrono::high_resolution_clock::time_point g_start_time;
static std::function<void(ImGuiID)> g_layout_function = nullptr;
static bool g_layout_applied = false;

// ============================================================
// Инициализация
// ============================================================

bool init_gui_library(const std::string& window_title, const int widthWindow, const int heightWindow) {
    if (!glfwInit()) {
        std::cerr << "Ошибка инициализации GLFW\n";
        return false;
    }

#ifdef __EMSCRIPTEN__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

    g_window = glfwCreateWindow(widthWindow, heightWindow, window_title.c_str(), nullptr, nullptr);
    if (!g_window) {
        std::cerr << "Ошибка создания окна GLFW\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(g_window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#ifndef __EMSCRIPTEN__
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();   // база: apply_site_style перекрывает её целиком
    apply_site_style();

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        // Окно, вытащенное за пределы главного, рисует ОС, и скруглённые углы
        // с полупрозрачным фоном там выглядят обрезанными.
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Шрифт с кириллицей. AddFontDefault() ставит ProggyClean, у которого
    // кириллических глифов нет вовсе – русские подписи выходили пустыми
    // квадратами, поэтому все учебные задачи подписаны по-английски.
    //
    // Roboto-Medium вшит в библиотеку массивом (src/roboto_medium_font.cpp), а не
    // читается с диска: путь к системному шрифту у Windows, Linux и macOS свой,
    // а в WebAssembly файловой системы нет совсем. Вшитый массив одинаково
    // работает на всех четырёх сборках.
    //
    // Диапазоны глифов не указываем: с версии 1.92 ImGui растеризует глиф по
    // требованию (external/imgui/docs/FONTS.md, «specifying glyph ranges is
    // unnecessary»), поэтому кириллица, знак градуса и тире берутся из шрифта
    // сами, без списка диапазонов, который пришлось бы держать живым.
    io.Fonts->AddFontFromMemoryCompressedBase85TTF(RobotoMedium_compressed_data_base85, 20.0f);

    ImGui_ImplGlfw_InitForOpenGL(g_window, true);
#ifdef __EMSCRIPTEN__
    // Убираем GLFW-scroll callback (ненадёжен в браузере с -sUSE_GLFW=3) и
    // регистрируем прямой Emscripten-callback, который сам кормит ImGui.
    glfwSetScrollCallback(g_window, nullptr);
    emscripten_set_wheel_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, EM_TRUE, emscripten_wheel_cb);
    ImGui_ImplOpenGL3_Init("#version 300 es");
#else
    ImGui_ImplOpenGL3_Init("#version 330");
#endif

    g_start_time = std::chrono::high_resolution_clock::now();
    return true;
}

// ============================================================
// Главный цикл
// ============================================================

bool gui_main_loop() {
    if (!g_window) return false;

#ifdef __EMSCRIPTEN__
    {
        int w = get_browser_width();
        int h = get_browser_height();
        glfwSetWindowSize(g_window, w, h);
    }
#endif

    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiID dockspace_id = ImGui::DockSpaceOverViewport();

    // Применяем разметку окон, если она задана
    if (g_layout_function && !g_layout_applied) {
#ifdef __EMSCRIPTEN__
        // В WASM нет imgui.ini — применяем всегда при старте
        bool should_apply = true;
#else
        // В нативной версии: применяем только если нет сохранённой разметки.
        // DockSpaceOverViewport() всегда создаёт узел, поэтому проверяем не на null,
        // а на то, что узел листовой (нет дочерних сплитов = разметка не строилась).
        ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspace_id);
        bool should_apply = !node || node->IsLeafNode();
#endif
        if (should_apply) {
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, ImGui::GetMainViewport()->Size);
            g_layout_function(dockspace_id);
            ImGui::DockBuilderFinish(dockspace_id);
        }
        g_layout_applied = true;
    }

    // Расчёт выполняется до рендеринга UI
    if (g_calculation_function) {
        g_calculation_function();
    }

    // Панель параметров
    ImGui::Begin("Parameters", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    for (auto& name : g_DrawOrder) {
        ImGui::PushID(name.c_str());
        ImGui::SetNextItemWidth(150.f);
        Parameter& param = g_parameters[name];
        switch (param.type) {
            case ParamType::Float: {
                float& val = std::get<float>(param.value);
                if (param.use_slider)
                    ImGui::SliderFloat(param.label.c_str(), &val, param.min_value, param.max_value, "%.3f");
                else
                    ImGui::InputFloat(param.label.c_str(), &val, param.step);
                break;
            }
            case ParamType::Int: {
                int& val = std::get<int>(param.value);
                if (param.use_slider)
                    ImGui::SliderInt(param.label.c_str(), &val, (int)param.min_value, (int)param.max_value, "%d");
                else
                    ImGui::InputInt(param.label.c_str(), &val);
                break;
            }
            case ParamType::Bool: {
                bool& val = std::get<bool>(param.value);
                ImGui::Checkbox(param.label.c_str(), &val);
                break;
            }
            case ParamType::String: {
                std::string& str = std::get<std::string>(param.value);
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "%s", str.c_str());
                if (ImGui::InputText(param.label.c_str(), buffer, sizeof(buffer)))
                    str = buffer;
                break;
            }
            case ParamType::Button:
                if (ImGui::Button(param.label.c_str(), ImVec2(250, 50)))
                    param.function();
                break;
            // Показания.
            //
            // Рисуются такой же рамкой, как поля ввода, но с флагом ReadOnly:
            // число видно, его можно выделить и скопировать в отчёт, а
            // изменить нельзя. Шаг 0 убирает кнопки «плюс-минус» – без них
            // поле и выглядит как показание прибора, а не как ручка.
            //
            // Прежде показания в задачах делали обычным add_float_param с
            // нулевым шагом: выглядело так же, но значение было редактируемым,
            // и правку молча затирал следующий кадр.
            //
            // Фон приглушён, чтобы показание отличалось от ручки не только
            // поведением: на пульте из двадцати строк это единственное, что
            // видно с проектора.
            case ParamType::OutputFloat:
            case ParamType::OutputInt: {
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.f, 0.f, 0.f, 0.25f));
                if (param.type == ParamType::OutputFloat) {
                    float val = std::get<float>(param.value);
                    ImGui::InputFloat(param.label.c_str(), &val, 0.f, 0.f, "%.4g",
                                      ImGuiInputTextFlags_ReadOnly);
                } else {
                    int val = std::get<int>(param.value);
                    ImGui::InputInt(param.label.c_str(), &val, 0, 0,
                                    ImGuiInputTextFlags_ReadOnly);
                }
                ImGui::PopStyleColor();
                break;
            }
        }
        ImGui::PopID();
    }

    ImGui::End();

    // Рендеринг графиков
    for (auto& [plot_name, plot_data] : g_plots) {
        ImGui::Begin(plot_name.c_str(), nullptr,
                     ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoScrollWithMouse);

        bool has_heatmap = !plot_data.heatmapVector.empty();

        // Кнопка экспорта. Стоит в самой библиотеке, а не в коде задачи: ученик,
        // которому нужно унести числа в отчёт, не станет дописывать её сам.
        // Показываем только когда есть что сохранять.
        if (!plot_data.historyVector.empty()) {
            if (ImGui::SmallButton("Сохранить CSV")) {
                save_plot_csv(plot_name);
            }
        }

        ImGuiCond cond = ImGuiCond_FirstUseEver;
        if (plot_data.scale_dirty) {
            cond = ImGuiCond_Always;
            plot_data.scale_dirty = false;
        }
        ImPlot::SetNextAxesLimits(plot_data.scale.x_min, plot_data.scale.x_max,
                                  plot_data.scale.y_min, plot_data.scale.y_max, cond);

        if (ImPlot::BeginPlot(plot_name.c_str(), ImVec2((float)plot_data.width, (float)plot_data.height))) {

            // Подписи осей. SetupAxes зовётся строго между BeginPlot и первой
            // отрисовкой, иначе ImPlot её проигнорирует. nullptr – ось без
            // подписи, поэтому пустая строка превращается именно в nullptr.
            if (!plot_data.x_label.empty() || !plot_data.y_label.empty()) {
                ImPlot::SetupAxes(plot_data.x_label.empty() ? nullptr : plot_data.x_label.c_str(),
                                  plot_data.y_label.empty() ? nullptr : plot_data.y_label.c_str());
            }

            // Тепловые карты — каждая со своей цветовой схемой
            for (auto& hm : plot_data.heatmapVector) {
                if (!hm.values.empty() && hm.rows > 0 && hm.cols > 0) {
                    ImPlot::PushColormap(hm.colormap);
                    const char* fmt = hm.label_fmt.empty() ? nullptr : hm.label_fmt.c_str();
                    ImPlot::PlotHeatmap(hm.label.c_str(),
                                        hm.values.data(),
                                        hm.rows, hm.cols,
                                        hm.scale_min, hm.scale_max,
                                        fmt,
                                        ImPlotPoint(plot_data.scale.x_min, plot_data.scale.y_max),
                                        ImPlotPoint(plot_data.scale.x_max, plot_data.scale.y_min));
                    ImPlot::PopColormap();
                }
            }

            // Тултип: при наведении показывает значения всех тепловых карт сразу
            if (has_heatmap && ImPlot::IsPlotHovered()) {
                ImPlotPoint mouse  = ImPlot::GetPlotMousePos();
                double x_range = plot_data.scale.x_max - plot_data.scale.x_min;
                double y_range = plot_data.scale.y_max - plot_data.scale.y_min;
                const auto& hm0 = plot_data.heatmapVector[0];
                int col = (int)((mouse.x - plot_data.scale.x_min) / x_range * hm0.cols);
                // Y перевёрнут: строка 0 данных — сверху (y_max), поэтому инвертируем
                int row = (int)((plot_data.scale.y_max - mouse.y) / y_range * hm0.rows);
                if (col >= 0 && col < hm0.cols && row >= 0 && row < hm0.rows) {
                    ImGui::BeginTooltip();
                    ImGui::Text("(x=%.1f, y=%.1f)", mouse.x, mouse.y);
                    for (auto& hm : plot_data.heatmapVector) {
                        if (!hm.values.empty())
                            ImGui::Text("  %s = %.4f", hm.label.c_str(),
                                        hm.values[row * hm.cols + col]);
                    }
                    ImGui::EndTooltip();
                }
            }

            // Сплошные линии
            for (auto& line : plot_data.lineVector) {
                ImPlot::SetNextLineStyle(line.color, line.size);
                ImPlot::PlotLine(line.label.c_str(),
                                 line.x_values.data(),
                                 line.y_values.data(),
                                 (int)line.x_values.size());
            }

            // Наборы точек (scatter) — батч-вызов на всю серию
            for (auto& pts : plot_data.pointsVector) {
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, pts.size,
                                           pts.color, IMPLOT_AUTO, pts.color);
                ImPlot::PlotScatter(pts.label.c_str(),
                                    pts.x_values.data(),
                                    pts.y_values.data(),
                                    (int)pts.x_values.size());
            }

            // Потоковые серии хранятся в кольцевом буфере. Параметр offset
            // ImPlot начинает чтение с самой старой точки после заполнения.
            for (auto& history : plot_data.historyVector) {
                if (history.count == 0) continue;
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, history.size,
                                           history.color, IMPLOT_AUTO, history.color);
                int offset = history.count == history.x_values.size()
                    ? static_cast<int>(history.next) : 0;
                ImPlot::PlotScatter(history.label.c_str(),
                                    history.x_values.data(),
                                    history.y_values.data(),
                                    static_cast<int>(history.count),
                                    0, offset);
            }

            // Одиночные точки
            for (auto& pt : plot_data.pointVector) {
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, pt.size,
                                           pt.color, IMPLOT_AUTO, pt.color);
                ImPlot::PlotScatter(pt.label.c_str(), &pt.x_value, &pt.y_value, 1);
            }

            // Круги с радиусом в координатах графика
            if (!plot_data.diskVector.empty()) {
                for (auto& disk : plot_data.diskVector) {
                    if (!disk.visible) continue;
                    ImPlot::FitPoint(ImPlotPoint(disk.x_value - disk.radius, disk.y_value - disk.radius));
                    ImPlot::FitPoint(ImPlotPoint(disk.x_value + disk.radius, disk.y_value + disk.radius));
                }
                ImPlot::PushPlotClipRect();
                ImDrawList* draw_list = ImPlot::GetPlotDrawList();
                for (auto& disk : plot_data.diskVector) {
                    if (!disk.visible) continue;
                    ImVec2 center = ImPlot::PlotToPixels(disk.x_value, disk.y_value);
                    ImVec2 edge   = ImPlot::PlotToPixels(disk.x_value + disk.radius, disk.y_value);
                    float px_r    = std::abs(edge.x - center.x);
                    if (px_r < 1.f) px_r = 1.f;
                    draw_list->AddCircleFilled(center, px_r,
                                               ImGui::ColorConvertFloat4ToU32(disk.color));
                }
                ImPlot::PopPlotClipRect();
            }

            ImPlot::EndPlot();
        }

        // Колорбары справа: по одному на каждую тепловую карту (стопкой)
        if (has_heatmap) {
            int  n_hm = (int)plot_data.heatmapVector.size();
            float cb_h = (float)plot_data.height / n_hm;
            ImGui::SameLine();
            ImGui::BeginGroup();
            for (auto& hm : plot_data.heatmapVector) {
                ImPlot::PushColormap(hm.colormap);
                ImPlot::ColormapScale(hm.label.c_str(), hm.scale_min, hm.scale_max,
                                      ImVec2(150, cb_h));
                ImPlot::PopColormap();
            }
            ImGui::EndGroup();
        }

        ImGui::SetNextItemWidth(150.f);
        ImGui::SliderInt("Width",  &plot_data.width,  100, 1600, "%d");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150.f);
        ImGui::SliderInt("Height", &plot_data.height, 100, 1200, "%d");

        ImGui::End();
    }

    ImGui::Render();

    int display_w, display_h;
    glfwGetFramebufferSize(g_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup);
    }

    glfwSwapBuffers(g_window);

    g_should_close = glfwWindowShouldClose(g_window);
    return !g_should_close;
}

// ============================================================
// run_gui_library — скрывает WASM-специфику от кода задачи
// ============================================================

#ifdef __EMSCRIPTEN__
static void emscripten_loop_step() { gui_main_loop(); }
#endif

void run_gui_library() {
#ifdef __EMSCRIPTEN__
    // emscripten_set_main_loop регистрирует колбэк и, при simulate_infinite_loop=1,
    // не возвращает управление — браузер сам управляет циклом.
    emscripten_set_main_loop(emscripten_loop_step, 0, 1);
#else
    while (gui_main_loop()) {
        sleep_ms(16);
    }
    shutdown_gui_library();
#endif
}

// ============================================================
// Завершение
// ============================================================

void shutdown_gui_library() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    if (g_window) {
        glfwDestroyWindow(g_window);
    }
    glfwTerminate();
}

// ============================================================
// Параметры
// ============================================================

void add_float_param(const std::string& name,
                     float initial_value, float min, float max, float step, bool use_slider) {
    Parameter param;
    param.name      = name;
    param.label     = name;
    param.type      = ParamType::Float;
    param.value     = initial_value;
    param.min_value = min;
    param.max_value = max;
    param.step      = step;
    param.use_slider = use_slider;
    g_parameters[name] = param;
    g_DrawOrder.push_back(name);
}

void add_int_param(const std::string& name,
                   int initial_value, int min, int max, int step, bool use_slider) {
    Parameter param;
    param.name      = name;
    param.label     = name;
    param.type      = ParamType::Int;
    param.value     = initial_value;
    param.min_value = (float)min;
    param.max_value = (float)max;
    param.step      = (float)step;
    param.use_slider = use_slider;
    g_parameters[name] = param;
    g_DrawOrder.push_back(name);
}

void add_bool_param(const std::string& name, bool initial_value) {
    Parameter param;
    param.name  = name;
    param.label = name;
    param.type  = ParamType::Bool;
    param.value = initial_value;
    g_parameters[name] = param;
    g_DrawOrder.push_back(name);
}

void add_string_param(const std::string& name, const std::string& initial_value) {
    Parameter param;
    param.name  = name;
    param.label = name;
    param.type  = ParamType::String;
    param.value = initial_value;
    g_parameters[name] = param;
    g_DrawOrder.push_back(name);
}

void add_button_param(const std::string& name, std::function<void()> function) {
    Parameter param;
    param.name     = name;
    param.label    = name;
    param.type     = ParamType::Button;
    param.value    = 0.f;  // не используется для кнопки
    param.function = function;
    g_parameters[name] = param;
    g_DrawOrder.push_back(name);
}

// Показания. Отдельного набора get/set у них нет: значение читается и пишется
// теми же get_float_param / set_float_param, что и у ручки, поэтому превратить
// ручку в показание – правка одного слова в add_*, а не всего кода задачи.
void add_output_float(const std::string& name, float initial_value) {
    Parameter param;
    param.name  = name;
    param.label = name;
    param.type  = ParamType::OutputFloat;
    param.value = initial_value;
    g_parameters[name] = param;
    g_DrawOrder.push_back(name);
}

void add_output_int(const std::string& name, int initial_value) {
    Parameter param;
    param.name  = name;
    param.label = name;
    param.type  = ParamType::OutputInt;
    param.value = initial_value;
    g_parameters[name] = param;
    g_DrawOrder.push_back(name);
}

float get_float_param(const std::string& name) {
    auto it = g_parameters.find(name);
    if (it != g_parameters.end() &&
        (it->second.type == ParamType::Float || it->second.type == ParamType::OutputFloat))
        return std::get<float>(it->second.value);
    std::cerr << "[gui_library] Предупреждение: параметр float '" << name << "' не найден\n";
    return 0.0f;
}

void set_float_param(const std::string& name, float value) {
    auto it = g_parameters.find(name);
    if (it != g_parameters.end() &&
        (it->second.type == ParamType::Float || it->second.type == ParamType::OutputFloat))
        it->second.value = value;
}

int get_int_param(const std::string& name) {
    auto it = g_parameters.find(name);
    if (it != g_parameters.end() &&
        (it->second.type == ParamType::Int || it->second.type == ParamType::OutputInt))
        return std::get<int>(it->second.value);
    std::cerr << "[gui_library] Предупреждение: параметр int '" << name << "' не найден\n";
    return 0;
}

void set_int_param(const std::string& name, int value) {
    auto it = g_parameters.find(name);
    if (it != g_parameters.end() &&
        (it->second.type == ParamType::Int || it->second.type == ParamType::OutputInt))
        it->second.value = value;
}

bool get_bool_param(const std::string& name) {
    auto it = g_parameters.find(name);
    if (it != g_parameters.end() && it->second.type == ParamType::Bool)
        return std::get<bool>(it->second.value);
    std::cerr << "[gui_library] Предупреждение: параметр bool '" << name << "' не найден\n";
    return false;
}

void set_bool_param(const std::string& name, bool value) {
    auto it = g_parameters.find(name);
    if (it != g_parameters.end() && it->second.type == ParamType::Bool)
        it->second.value = value;
}

std::string get_string_param(const std::string& name) {
    auto it = g_parameters.find(name);
    if (it != g_parameters.end() && it->second.type == ParamType::String)
        return std::get<std::string>(it->second.value);
    std::cerr << "[gui_library] Предупреждение: параметр string '" << name << "' не найден\n";
    return "";
}

void set_string_param(const std::string& name, const std::string& value) {
    auto it = g_parameters.find(name);
    if (it != g_parameters.end() && it->second.type == ParamType::String)
        it->second.value = value;
}

// ============================================================
// Графики
// ============================================================

// Внутренняя перегрузка: Scale больше не публичный тип, но реализации удобно.
static void create_plot_scaled(const std::string& name, const Scale& scale, int width, int height) {
    g_plots[name] = PlotData(scale, width, height);
}

void create_plot(const std::string& name,
                 float x_min, float x_max, float y_min, float y_max,
                 int width, int height) {
    create_plot_scaled(name, Scale(x_min, x_max, y_min, y_max), width, height);
}

void set_plot_scale(const std::string& name, float x_min, float x_max, float y_min, float y_max) {
    auto it = g_plots.find(name);
    if (it != g_plots.end()) {
        it->second.scale.x_min  = x_min;
        it->second.scale.x_max  = x_max;
        it->second.scale.y_min  = y_min;
        it->second.scale.y_max  = y_max;
        it->second.scale_dirty  = true;
    }
}

// ============================================================
// Экспорт данных
// ============================================================

// Экранирование поля CSV: кавычки удваиваются, всё поле берётся в кавычки.
// Подпись серии – это формула вроде "y = A1*sin(w1*t) + A2*sin(w2*t)", в ней
// может оказаться запятая, и без кавычек строка развалилась бы на две колонки.
static std::string csv_field(const std::string& text) {
    std::string out = "\"";
    for (char c : text) {
        if (c == '"') out += '"';
        out += c;
    }
    out += '"';
    return out;
}

static std::string csv_number(float value) {
    char buf[32];
    // %.9g – столько десятичных знаков, сколько нужно, чтобы float пережил
    // запись и чтение без потери. Точка как разделитель: это стандарт CSV,
    // Excel с русскими настройками откроет через «Данные – Из текста».
    snprintf(buf, sizeof(buf), "%.9g", value);
    return buf;
}

#ifdef __EMSCRIPTEN__
// В браузере файловой системы нет: отдаём содержимое на скачивание. Blob и
// временная ссылка – единственный способ, который не требует ни сервера, ни
// разрешений. Ссылку освобождаем позже: если сделать это сразу, часть браузеров
// не успевает начать скачивание.
EM_JS(void, browser_download_text, (const char* name, const char* data, int len), {
    var bytes = HEAPU8.slice(data, data + len);
    var url = URL.createObjectURL(new Blob([bytes], { type: 'text/csv;charset=utf-8' }));
    var a = document.createElement('a');
    a.href = url;
    a.download = UTF8ToString(name);
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    setTimeout(function () { URL.revokeObjectURL(url); }, 2000);
});
#endif

bool save_plot_csv(const std::string& plot_name, const std::string& filename) {
    auto it = g_plots.find(plot_name);
    if (it == g_plots.end()) {
        std::cerr << "[gui_library] Предупреждение: график '" << plot_name << "' не найден\n";
        return false;
    }
    const PlotData& plot = it->second;
    if (plot.historyVector.empty()) {
        std::cerr << "[gui_library] У графика '" << plot_name
                  << "' нет накопленной истории: сохранять нечего. "
                     "История появляется от add_plot_history_point.\n";
        return false;
    }

    // BOM: без него Excel читает UTF-8 как системную кодировку, и русские
    // подписи серий превращаются в мусор.
    std::string csv = "\xEF\xBB\xBF" "серия,x,y\n";

    for (const PlotHistory& history : plot.historyVector) {
        if (history.count == 0) continue;
        // Кольцевой буфер: пока он не заполнен, точки лежат подряд с нуля;
        // после заполнения самая старая стоит на месте следующей записи.
        const size_t capacity = history.x_values.size();
        const size_t offset = (history.count == capacity) ? history.next : 0;
        const std::string label = csv_field(history.label);
        for (size_t i = 0; i < history.count; ++i) {
            const size_t idx = (offset + i) % capacity;
            csv += label;
            csv += ',';
            csv += csv_number(history.x_values[idx]);
            csv += ',';
            csv += csv_number(history.y_values[idx]);
            csv += '\n';
        }
    }

    const std::string name = filename.empty() ? plot_name + ".csv" : filename;

#ifdef __EMSCRIPTEN__
    browser_download_text(name.c_str(), csv.data(), (int)csv.size());
    return true;
#else
    std::ofstream out(name, std::ios::binary);
    if (!out) {
        std::cerr << "[gui_library] Не удалось открыть файл '" << name << "' для записи\n";
        return false;
    }
    out.write(csv.data(), (std::streamsize)csv.size());
    if (!out) {
        std::cerr << "[gui_library] Ошибка записи в '" << name << "'\n";
        return false;
    }
    out.close();
    // Путь печатаем целиком: иначе ученик не найдёт файл – рабочий каталог
    // зависит от того, запущено из Visual Studio, из скрипта или двойным щелчком.
    std::error_code ec;
    const auto full = std::filesystem::absolute(name, ec);
    std::cout << "[gui_library] Сохранено: " << (ec ? name : full.string()) << "\n";
    return true;
#endif
}

void set_plot_axes(const std::string& plot_name,
                   const std::string& x_label, const std::string& y_label) {
    auto it = g_plots.find(plot_name);
    if (it == g_plots.end()) {
        std::cerr << "[gui_library] Предупреждение: график '" << plot_name << "' не найден\n";
        return;
    }
    it->second.x_label = x_label;
    it->second.y_label = y_label;
}

void add_plot_disk(const std::string& plot_name, float x, float y, float radius,
                   const std::string& label, const ImVec4& color) {
    auto it = g_plots.find(plot_name);
    if (it == g_plots.end()) {
        std::cerr << "[gui_library] Предупреждение: график '" << plot_name << "' не найден\n";
        return;
    }
    PlotDisk data;
    data.x_value = x;
    data.y_value = y;
    data.radius  = radius;
    data.label   = label;
    data.color   = color;
    it->second.diskVector.push_back(data);
}

void add_plot_point(const std::string& plot_name, float x, float y,
                    const std::string& label, const ImVec4& color, float size) {
    auto it = g_plots.find(plot_name);
    if (it == g_plots.end()) {
        std::cerr << "[gui_library] Предупреждение: график '" << plot_name << "' не найден\n";
        return;
    }
    PlotPoint data;
    data.x_value = x;
    data.y_value = y;
    data.label   = label;
    data.color   = color;
    data.size    = size;
    it->second.pointVector.push_back(data);
}

void add_plot_points(const std::string& plot_name,
                     const std::vector<float>& x, const std::vector<float>& y,
                     const std::string& label, const ImVec4& color, float size) {
    auto it = g_plots.find(plot_name);
    if (it == g_plots.end()) {
        std::cerr << "[gui_library] Предупреждение: график '" << plot_name << "' не найден\n";
        return;
    }
    PlotPoints data;
    data.x_values = x;
    data.y_values = y;
    data.label    = label;
    data.color    = color;
    data.size     = size;
    it->second.pointsVector.push_back(data);
}

void add_plot_points(const std::string& plot_name,
                     const std::vector<double>& x, const std::vector<double>& y,
                     const std::string& label, const ImVec4& color, float size) {
    // Сужение double -> float здесь намеренное: графики хранят float.
    // Явный static_cast вместо конструктора по диапазону – иначе MSVC на
    // каждой сборке выдаёт C4244, и ученик привыкает не читать предупреждения.
    std::vector<float> xf(x.size()), yf(y.size());
    for (size_t i = 0; i < x.size(); ++i) xf[i] = static_cast<float>(x[i]);
    for (size_t i = 0; i < y.size(); ++i) yf[i] = static_cast<float>(y[i]);
    add_plot_points(plot_name, xf, yf, label, color, size);
}

void add_plot_line(const std::string& plot_name,
                   const std::vector<float>& x, const std::vector<float>& y,
                   const std::string& label, const ImVec4& color, float size) {
    auto it = g_plots.find(plot_name);
    if (it == g_plots.end()) {
        std::cerr << "[gui_library] Предупреждение: график '" << plot_name << "' не найден\n";
        return;
    }
    PlotLine data;
    data.x_values = x;
    data.y_values = y;
    data.label    = label;
    data.color    = color;
    data.size     = size;
    it->second.lineVector.push_back(data);
}

void add_plot_line(const std::string& plot_name,
                   const std::vector<double>& x, const std::vector<double>& y,
                   const std::string& label, const ImVec4& color, float size) {
    // Сужение double -> float здесь намеренное: графики хранят float.
    // Явный static_cast вместо конструктора по диапазону – иначе MSVC на
    // каждой сборке выдаёт C4244, и ученик привыкает не читать предупреждения.
    std::vector<float> xf(x.size()), yf(y.size());
    for (size_t i = 0; i < x.size(); ++i) xf[i] = static_cast<float>(x[i]);
    for (size_t i = 0; i < y.size(); ++i) yf[i] = static_cast<float>(y[i]);
    add_plot_line(plot_name, xf, yf, label, color, size);
}

void add_plot_history_point(const std::string& plot_name,
                            float x, float y,
                            const std::string& label,
                            const ImVec4& color, float size,
                            size_t max_points) {
    auto plot_it = g_plots.find(plot_name);
    if (plot_it == g_plots.end()) {
        std::cerr << "[gui_library] Предупреждение: график '" << plot_name << "' не найден\n";
        return;
    }
    if (max_points == 0) {
        std::cerr << "[gui_library] Предупреждение: max_points должен быть больше нуля\n";
        return;
    }

    auto& histories = plot_it->second.historyVector;
    auto history_it = std::find_if(histories.begin(), histories.end(),
        [&label](const PlotHistory& history) { return history.label == label; });

    if (history_it == histories.end()) {
        PlotHistory history;
        history.label = label;
        history.color = color;
        history.size = size;
        history.x_values.resize(max_points);
        history.y_values.resize(max_points);
        histories.push_back(std::move(history));
        history_it = std::prev(histories.end());
    }

    PlotHistory& history = *history_it;
    history.color = color;
    history.size = size;
    if (history.x_values.size() != max_points) {
        // Изменение ёмкости начинает новую историю: это предсказуемее, чем
        // частичное копирование кольцевого буфера с другой конфигурацией.
        history.x_values.assign(max_points, 0.f);
        history.y_values.assign(max_points, 0.f);
        history.next = 0;
        history.count = 0;
    }

    history.x_values[history.next] = x;
    history.y_values[history.next] = y;
    history.next = (history.next + 1) % max_points;
    history.count = std::min(history.count + 1, max_points);
}

void clear_plot_history(const std::string& plot_name) {
    auto it = g_plots.find(plot_name);
    if (it != g_plots.end())
        it->second.historyVector.clear();
}

void clear_plot_history(const std::string& plot_name, const std::string& label) {
    auto it = g_plots.find(plot_name);
    if (it == g_plots.end()) return;
    auto& histories = it->second.historyVector;
    histories.erase(std::remove_if(histories.begin(), histories.end(),
        [&label](const PlotHistory& history) { return history.label == label; }),
        histories.end());
}

void add_plot_heatmap(const std::string& plot_name,
                      const std::vector<float>& values,
                      int rows, int cols,
                      const std::string& label,
                      double scale_min, double scale_max,
                      int colormap,
                      const std::string& label_fmt) {
    auto it = g_plots.find(plot_name);
    if (it == g_plots.end()) {
        std::cerr << "[gui_library] Предупреждение: график '" << plot_name << "' не найден\n";
        return;
    }
    Heatmap data;
    data.values    = values;
    data.rows      = rows;
    data.cols      = cols;
    data.label     = label;
    data.scale_min = scale_min;
    data.scale_max = scale_max;
    data.colormap  = colormap;
    data.label_fmt = label_fmt;
    it->second.heatmapVector.push_back(data);
}

void clear_plot(const std::string& plot_name) {
    auto it = g_plots.find(plot_name);
    if (it != g_plots.end())
        it->second.clear();
}

// ============================================================
// Расчёт
// ============================================================

void set_calculation_function(std::function<void()> calc_func) {
    g_calculation_function = calc_func;
}

// ============================================================
// Разметка окон
// ============================================================

void set_auto_layout(float params_ratio) {
    g_layout_function = [params_ratio](ImGuiID dockspace_id) {
        ImGuiID left_id, right_id;
        ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left,
                                    params_ratio, &left_id, &right_id);
        ImGui::DockBuilderDockWindow("Parameters", left_id);
        // Все графики попадают в один правый узел — ImGui сделает из них вкладки
        for (auto& [plot_name, unused] : g_plots) {
            ImGui::DockBuilderDockWindow(plot_name.c_str(), right_id);
        }
    };
}

void set_default_layout(std::function<void(ImGuiID)> layout_fn) {
    g_layout_function = layout_fn;
}

void layout_split_left(ImGuiID node, float ratio, ImGuiID* out_left, ImGuiID* out_right) {
    ImGui::DockBuilderSplitNode(node, ImGuiDir_Left, ratio, out_left, out_right);
}

void layout_split_up(ImGuiID node, float ratio, ImGuiID* out_top, ImGuiID* out_bottom) {
    ImGui::DockBuilderSplitNode(node, ImGuiDir_Up, ratio, out_top, out_bottom);
}

void layout_dock(const std::string& name, ImGuiID node) {
    ImGui::DockBuilderDockWindow(name.c_str(), node);
}

// ============================================================
// Утилиты
// ============================================================

void sleep_ms(int milliseconds) {
#ifndef __EMSCRIPTEN__
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
#endif
}

double get_time() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - g_start_time);
    return duration.count() / 1000000.0;
}
