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


// Глобальные переменные для хранения состояниякак изменить 
static GLFWwindow* g_window = nullptr;
static std::map<std::string, Parameter> g_parameters;
static std::vector<std::string> g_DrawOrder;
static std::map<std::string, PlotData> g_plots;
static std::function<void()> g_calculation_function = nullptr;
static bool g_should_close = false;
static std::chrono::high_resolution_clock::time_point g_start_time;
static ImGuiID g_dockspace_id = 0;

// Инициализация библиотеки
bool init_gui_library(const std::string& window_title, const int widthWindow, const int heightWindow) {
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
    g_window = glfwCreateWindow(widthWindow, heightWindow, window_title.c_str(), nullptr, nullptr);
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

    ImFont* font = nullptr;
    ImFontConfig font_cfg;
    font_cfg.SizePixels = 20.0f; // Устанавливаем размер 20
    
    font = io.Fonts->AddFontDefault(&font_cfg);

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
    ImGui::Begin("Parameters", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    
       // Выполняем функцию расчета если она установлена
    if (g_calculation_function) {
        g_calculation_function();
    }

    for (auto& name : g_DrawOrder) {
        ImGui::PushID(name.c_str());
        ImGui::SetNextItemWidth(150.f);
        Parameter& param = g_parameters[name];
        switch (param.type) {
            case ParamType::Float:
                if (param.use_slider) {
                    ImGui::SliderFloat(param.label.c_str(), &param.float_value, 
                                param.min_value, param.max_value, "%.3f");
                    
                } 
                else {
                    ImGui::InputFloat(param.label.c_str(), &param.float_value, param.step);
                }
                break;
            case ParamType::Int:
                if (param.use_slider) {
                    ImGui::SliderInt(param.label.c_str(), &param.int_value, 
                                (int)param.min_value, (int)param.max_value, "%d");
                } 
                else {
                    ImGui::InputInt(param.label.c_str(), &param.int_value);
                }
                break;
            case ParamType::Bool:
                ImGui::Checkbox(param.label.c_str(), &param.bool_value);
                break;
            case ParamType::String:
                char buffer[256];
                strcpy(buffer, param.string_value.c_str());
                if (ImGui::InputText(param.label.c_str(), buffer, sizeof(buffer))) {
                    param.string_value = buffer;
                }
                break;
            case ParamType::Button:
                if (ImGui::Button(param.label.c_str(), ImVec2(250, 50))) {
                    param.function();
                }
                break;

        } 
        ImGui::PopID();
    }
    
    ImGui::End();


    for (auto& [plot_name, plot_data] : g_plots) {
        ImGui::Begin(plot_name.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImPlot::SetNextAxesLimits(plot_data.scale.x_min, plot_data.scale.x_max, plot_data.scale.y_min, plot_data.scale.y_max, ImGuiCond_FirstUseEver);
        if (ImPlot::BeginPlot(plot_name.c_str(), ImVec2(plot_data.scale.width, plot_data.scale.height))) {
            for (auto& line_data : plot_data.lineVector) {
                    ImPlot::SetNextLineStyle(line_data.color, line_data.size);
                    ImPlot::PlotLine(line_data.label.c_str(), 
                                        line_data.x_values.data(), 
                                        line_data.y_values.data(), 
                                        line_data.x_values.size());
                                        
            }

            for (auto& scatterline_data : plot_data.scatterlineVector) {
                    for (size_t i = 0; i < scatterline_data.x_values.size(); ++i) {
                        double x = scatterline_data.x_values[i];
                        double y = scatterline_data.y_values[i];

                        ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, scatterline_data.size, 
                                                scatterline_data.color,
                                                IMPLOT_AUTO, 
                                                scatterline_data.color); 
                        ImPlot::PlotScatter(scatterline_data.label.c_str(), &x, &y, 1);
                        
                    }                       
            }

            for (auto& scatter_data : plot_data.scatterVector) {
                    float x = scatter_data.x_values;
                    float y = scatter_data.y_values;
                    ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, scatter_data.size, 
                                                        scatter_data.color,  // красная заливка
                                                        IMPLOT_AUTO, 
                                                        scatter_data.color); // чёрная обводка
                    ImPlot::PlotScatter(scatter_data.label.c_str(), &x, &y, 1);                     
            }
            ImPlot::EndPlot();
        }
        ImGui::SetNextItemWidth(150.f);
        ImGui::SliderInt("Width", &plot_data.scale.width, 
                                100, 1000, "%d");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150.f);
        ImGui::SliderInt("Height", &plot_data.scale.height, 
                                100, 1000, "%d");
                                
        ImGui::End();
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

void add_float_param(const std::string& name, 
                    float initial_value, float min, float max , float step , bool use_slider) {
    Parameter param;
    param.name = name;
    param.label = name;
    param.type = ParamType::Float;
    param.float_value = initial_value;
    param.min_value = min;
    param.max_value = max;
    param.step = step;
    param.use_slider = use_slider;
    g_parameters[name] = param;
    g_DrawOrder.push_back(name);
}


void add_int_param(const std::string& name, 
                  int initial_value, int min, int max, int step , bool use_slider) {
    Parameter param;
    param.name = name;
    param.label = name;
    param.type = ParamType::Int;
    param.int_value = initial_value;
    param.min_value = (float)min;
    param.max_value = (float)max;
    param.step = (float)step;
    param.use_slider = use_slider;
    g_parameters[name] = param;
    g_DrawOrder.push_back(name);
}

void add_bool_param(const std::string& name, bool initial_value) {
    Parameter param;
    param.name = name;
    param.label = name;
    param.type = ParamType::Bool;
    param.bool_value = initial_value;
    g_parameters[name] = param;
    g_DrawOrder.push_back(name);
}

void add_string_param(const std::string& name, const std::string& initial_value) {
    Parameter param;
    param.name = name;
    param.label = name;
    param.type = ParamType::String;
    param.string_value = initial_value;
    g_parameters[name] = param;
    g_DrawOrder.push_back(name);
}

void add_button_param(const std::string& name, std::function<void()> function) {
    Parameter param;
    param.name = name;
    param.label = name;
    param.type = ParamType::Button;
    param.function = function;
    g_parameters[name] = param;
    g_DrawOrder.push_back(name);
}

float get_float_param(const std::string& name) {
    auto it = g_parameters.find(name);
    if (it != g_parameters.end() && it->second.type == ParamType::Float) {
        return it->second.float_value;
    }
    return 0.0f;
}

// void set_float_param(const std::string& name, float value) {
//     auto it = g_parameters.find(name);
//     if (it != g_parameters.end() && it->second.type == ParamType::Bool) {
//         it->second.float_value = value;
//     }
// }

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

void set_bool_param(const std::string& name, bool value) {
    auto it = g_parameters.find(name);
    if (it != g_parameters.end() && it->second.type == ParamType::Bool) {
        it->second.bool_value = value;
    }
}

std::string get_string_param(const std::string& name) {
    auto it = g_parameters.find(name);
    if (it != g_parameters.end() && it->second.type == ParamType::String) {
        return it->second.string_value;
    }
    return "";
}



// === ФУНКЦИИ ДЛЯ РАБОТЫ С ГРАФИКАМИ ===

void create_plot(const std::string& name, const Scale& scale) {
    g_plots[name] = PlotData(scale);
}

void add_plot_scatter(const std::string& plot_name, const float& x, const float& y, 
                    const std::string& label, const ImVec4& color, const float& size) {
    auto it = g_plots.find(plot_name);
    if (it != g_plots.end()) {
        Scatter data;
        data.x_values = x;
        data.y_values = y;
        data.label = label;
        data.color = color;
        data.size = size;
        it->second.scatterVector.push_back(data);
    }
}

void add_plot_scatterline(const std::string& plot_name, const std::vector<float>& x, const std::vector<float>& y, 
                    const std::string& label, const ImVec4& color, const float& size) {
    auto it = g_plots.find(plot_name);
    if (it != g_plots.end()) {
        ScatterLine data;
        data.x_values = x;
        data.y_values = y;
        data.label = label;
        data.color = color;
        data.size = size;
        it->second.scatterlineVector.push_back(data);
    }
}

void add_plot_line(const std::string& plot_name, const std::vector<float>& x, const std::vector<float>& y, 
                    const std::string& label, const ImVec4& color, const float& size) {
    auto it = g_plots.find(plot_name);
    if (it != g_plots.end()) {
        FillLine data;
        data.x_values = x;
        data.y_values = y;
        data.label = label;
        data.color = color;
        data.size = size;
        it->second.lineVector.push_back(data);
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



// int get_plot_data_size(const std::string& plot_name) {
//     auto it = g_plots.find(plot_name);
//     if (it != g_plots.end() && !it->second.empty()) {
//         return it->second[0].x_values.size();
//     }
//     return 0;
// }

// void fill_plot_data(const std::string& plot_name, int index, float x, float y) {
//     auto it = g_plots.find(plot_name);
//     if (it != g_plots.end() && !it->second.empty()) {
//         auto& plot_data = it->second[0];
//         if (index < plot_data.x_values.size()) {
//             plot_data.x_values[index] = x;
//             plot_data.y_values[index] = y;
//         }
//     }
// }

// === УТИЛИТЫ ===


void sleep_ms(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

double get_time() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - g_start_time);
    return duration.count() / 1000000.0;
}
