#include "gui_library.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include <GLFW/glfw3.h>
#include <map>
#include <chrono>
#include <thread>
#include <iostream>

// Глобальные переменные для хранения состояния
static GLFWwindow* g_window = nullptr;
static std::map<std::string, Parameter> g_parameters;
static std::map<std::string, std::vector<PlotData>> g_plots;
static std::function<void()> g_calculation_function = nullptr;
static bool g_should_close = false;
static std::chrono::high_resolution_clock::time_point g_start_time;
static ImGuiID g_dockspace_id = 0;

// Инициализация библиотеки
bool init_gui_library(const std::string& window_title) {
    // Инициализация GLFW
    if (!glfwInit()) {
        std::cerr << "Ошибка инициализации GLFW" << std::endl;
        return false;
    }

    // Настройка OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Создание окна
    g_window = glfwCreateWindow(1200, 800, window_title.c_str(), nullptr, nullptr);
    if (!g_window) {
        std::cerr << "Ошибка создания окна GLFW" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(g_window);
    glfwSwapInterval(1); // V-Sync

    // Инициализация ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    
    ImGuiIO& io = ImGui::GetIO();
    // Включаем поддержку docking и viewports
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Включаем навигацию с клавиатуры

    // Настройка стиля
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Загрузка шрифта с поддержкой кириллицы
    // Пытаемся загрузить системный шрифт с поддержкой кириллицы
    ImFont* font = nullptr;
    
    // Попробуем загрузить шрифт из системы Windows
    font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf", 16.0f);
    if (!font) {
        font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", 16.0f);
    }
    if (!font) {
        font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/calibri.ttf", 16.0f);
    }
    if (!font) {
        // Если не удалось загрузить системный шрифт, используем встроенный
        font = io.Fonts->AddFontDefault();
        std::cerr << "Предупреждение: Не удалось загрузить шрифт с поддержкой кириллицы. Используется стандартный шрифт." << std::endl;
    }

    // Инициализация бэкендов
    ImGui_ImplGlfw_InitForOpenGL(g_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    g_start_time = std::chrono::high_resolution_clock::now();
    return true;
}

// Основной цикл приложения
bool gui_main_loop() {
    if (!g_window) return false;

    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Создаем докируемое пространство
    ImGui::DockSpaceOverViewport();

    // Левая панель с параметрами
    ImGui::Begin("Параметры", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    
    for (auto& [name, param] : g_parameters) {
        ImGui::PushID(name.c_str());
        
        switch (param.type) {
            case ParamType::Float:
                ImGui::SliderFloat(param.label.c_str(), &param.float_value, 
                                 param.min_value, param.max_value, "%.3f", param.step);
                break;
            case ParamType::Int:
                ImGui::SliderInt(param.label.c_str(), &param.int_value, 
                               (int)param.min_value, (int)param.max_value, "%d");
                break;
            case ParamType::Bool:
                ImGui::Checkbox(param.label.c_str(), &param.bool_value);
                break;
            case ParamType::String:
                char buffer[256];
                strcpy_s(buffer, param.string_value.c_str());
                if (ImGui::InputText(param.label.c_str(), buffer, sizeof(buffer))) {
                    param.string_value = buffer;
                }
                break;
        }
        
        ImGui::PopID();
    }
    
    ImGui::End();

    // Правая панель с графиками
    for (auto& [plot_name, plot_data_vector] : g_plots) {
        ImGui::Begin(plot_name.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        
        if (ImPlot::BeginPlot(plot_name.c_str())) {
            for (auto& plot_data : plot_data_vector) {
                if (plot_data.visible && !plot_data.x_values.empty()) {
                    ImPlot::PlotLine(plot_data.label.c_str(), 
                                   plot_data.x_values.data(), 
                                   plot_data.y_values.data(), 
                                   plot_data.x_values.size());
                }
            }
            ImPlot::EndPlot();
        }
        
        ImGui::End();
    }

    // Выполняем функцию расчета если она установлена
    if (g_calculation_function) {
        g_calculation_function();
    }

    ImGui::Render();
    
    int display_w, display_h;
    glfwGetFramebufferSize(g_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Обновление и рендеринг дополнительных платформенных окон
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    glfwSwapBuffers(g_window);
    
    g_should_close = glfwWindowShouldClose(g_window);
    return !g_should_close;
}

// Завершение работы
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

// === ФУНКЦИИ ДЛЯ РАБОТЫ С ПАРАМЕТРАМИ ===

void add_float_param(const std::string& name, const std::string& label, 
                    float initial_value, float min, float max, float step) {
    Parameter param;
    param.name = name;
    param.label = label;
    param.type = ParamType::Float;
    param.float_value = initial_value;
    param.min_value = min;
    param.max_value = max;
    param.step = step;
    g_parameters[name] = param;
}

void add_int_param(const std::string& name, const std::string& label, 
                  int initial_value, int min, int max, int step) {
    Parameter param;
    param.name = name;
    param.label = label;
    param.type = ParamType::Int;
    param.int_value = initial_value;
    param.min_value = (float)min;
    param.max_value = (float)max;
    param.step = (float)step;
    g_parameters[name] = param;
}

void add_bool_param(const std::string& name, const std::string& label, bool initial_value) {
    Parameter param;
    param.name = name;
    param.label = label;
    param.type = ParamType::Bool;
    param.bool_value = initial_value;
    g_parameters[name] = param;
}

void add_string_param(const std::string& name, const std::string& label, const std::string& initial_value) {
    Parameter param;
    param.name = name;
    param.label = label;
    param.type = ParamType::String;
    param.string_value = initial_value;
    g_parameters[name] = param;
}

float get_float_param(const std::string& name) {
    auto it = g_parameters.find(name);
    if (it != g_parameters.end() && it->second.type == ParamType::Float) {
        return it->second.float_value;
    }
    return 0.0f;
}

int get_int_param(const std::string& name) {
    auto it = g_parameters.find(name);
    if (it != g_parameters.end() && it->second.type == ParamType::Int) {
        return it->second.int_value;
    }
    return 0;
}

bool get_bool_param(const std::string& name) {
    auto it = g_parameters.find(name);
    if (it != g_parameters.end() && it->second.type == ParamType::Bool) {
        return it->second.bool_value;
    }
    return false;
}

std::string get_string_param(const std::string& name) {
    auto it = g_parameters.find(name);
    if (it != g_parameters.end() && it->second.type == ParamType::String) {
        return it->second.string_value;
    }
    return "";
}

// === ФУНКЦИИ ДЛЯ РАБОТЫ С ГРАФИКАМИ ===

void create_plot(const std::string& name, const std::string& title) {
    g_plots[name] = std::vector<PlotData>();
}

void add_plot_data(const std::string& plot_name, const std::vector<float>& x, 
                  const std::vector<float>& y, const std::string& label) {
    auto it = g_plots.find(plot_name);
    if (it != g_plots.end()) {
        PlotData data;
        data.x_values = x;
        data.y_values = y;
        data.label = label;
        it->second.push_back(data);
    }
}

void clear_plot(const std::string& plot_name) {
    auto it = g_plots.find(plot_name);
    if (it != g_plots.end()) {
        it->second.clear();
    }
}

// === ФУНКЦИИ ДЛЯ РАСЧЕТОВ ===

void set_calculation_function(std::function<void()> calc_func) {
    g_calculation_function = calc_func;
}

int get_plot_data_size(const std::string& plot_name) {
    auto it = g_plots.find(plot_name);
    if (it != g_plots.end() && !it->second.empty()) {
        return it->second[0].x_values.size();
    }
    return 0;
}

void fill_plot_data(const std::string& plot_name, int index, float x, float y) {
    auto it = g_plots.find(plot_name);
    if (it != g_plots.end() && !it->second.empty()) {
        auto& plot_data = it->second[0];
        if (index < plot_data.x_values.size()) {
            plot_data.x_values[index] = x;
            plot_data.y_values[index] = y;
        }
    }
}

// === УТИЛИТЫ ===

std::vector<float> create_data_array(int size) {
    return std::vector<float>(size, 0.0f);
}

void sleep_ms(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

double get_time() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - g_start_time);
    return duration.count() / 1000000.0;
}
