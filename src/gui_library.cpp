#include "gui_library.h"
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

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImFontConfig font_cfg;
    font_cfg.SizePixels = 20.0f;
    io.Fonts->AddFontDefault(&font_cfg);

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

        ImGuiCond cond = ImGuiCond_FirstUseEver;
        if (plot_data.scale_dirty) {
            cond = ImGuiCond_Always;
            plot_data.scale_dirty = false;
        }
        ImPlot::SetNextAxesLimits(plot_data.scale.x_min, plot_data.scale.x_max,
                                  plot_data.scale.y_min, plot_data.scale.y_max, cond);

        if (ImPlot::BeginPlot(plot_name.c_str(), ImVec2((float)plot_data.width, (float)plot_data.height))) {

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

float get_float_param(const std::string& name) {
    auto it = g_parameters.find(name);
    if (it != g_parameters.end() && it->second.type == ParamType::Float)
        return std::get<float>(it->second.value);
    std::cerr << "[gui_library] Предупреждение: параметр float '" << name << "' не найден\n";
    return 0.0f;
}

void set_float_param(const std::string& name, float value) {
    auto it = g_parameters.find(name);
    if (it != g_parameters.end() && it->second.type == ParamType::Float)
        it->second.value = value;
}

int get_int_param(const std::string& name) {
    auto it = g_parameters.find(name);
    if (it != g_parameters.end() && it->second.type == ParamType::Int)
        return std::get<int>(it->second.value);
    std::cerr << "[gui_library] Предупреждение: параметр int '" << name << "' не найден\n";
    return 0;
}

void set_int_param(const std::string& name, int value) {
    auto it = g_parameters.find(name);
    if (it != g_parameters.end() && it->second.type == ParamType::Int)
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

void create_plot(const std::string& name, const Scale& scale, int width, int height) {
    g_plots[name] = PlotData(scale, width, height);
}

void create_plot(const std::string& name,
                 float x_min, float x_max, float y_min, float y_max,
                 int width, int height) {
    create_plot(name, Scale(x_min, x_max, y_min, y_max), width, height);
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
    std::vector<float> xf(x.begin(), x.end());
    std::vector<float> yf(y.begin(), y.end());
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
    std::vector<float> xf(x.begin(), x.end());
    std::vector<float> yf(y.begin(), y.end());
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
