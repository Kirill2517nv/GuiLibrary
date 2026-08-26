cmake_minimum_required(VERSION 3.16)
project(GUI_Library)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 32-bit индексы для ImGui (нужно для Emscripten + docking, чтобы не переполнять 16-bit лимит)
add_compile_definitions(ImDrawIdx=unsigned\ int)

# Вспомогательная функция для настройки WASM-сборки заданий
include(cmake/EmscriptenTask.cmake)

# Настройки для Windows
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Поиск пакетов (не нужно для Emscripten)
if(NOT EMSCRIPTEN)
    find_package(OpenGL REQUIRED)
endif()


# ============================================================
# ImGui
# ============================================================
add_library(imgui
    external/imgui/imgui.cpp
    external/imgui/imgui_demo.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends
)

if(EMSCRIPTEN)
    target_link_libraries(imgui PUBLIC)
else()
    target_link_libraries(imgui PUBLIC glfw OpenGL::GL)
endif()

# ============================================================
# ImPlot
# ============================================================
add_library(implot
    external/implot/implot.cpp
    external/implot/implot_items.cpp
    external/implot/implot_demo.cpp
)

target_include_directories(implot PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/external/implot
)

target_link_libraries(implot PUBLIC imgui)

# ============================================================
# GLFW
# ============================================================
if(NOT EMSCRIPTEN)
    add_subdirectory(external/glfw)
endif()

# ============================================================
# Основная библиотека
# ============================================================
add_library(gui_library
    src/gui_library.cpp
)

target_include_directories(gui_library PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include
)

if(EMSCRIPTEN)
    target_link_libraries(gui_library
        imgui
        implot
    )
else()
    target_link_libraries(gui_library
        imgui
        implot
        glfw
        OpenGL::GL
    )
endif()

# ============================================================
# Задания (можно выборочно отключать через -DBUILD_TASK_N=OFF)
# ============================================================
option(BUILD_TASK_0  "Собирать Task_0 (LBM)"               ON)
option(BUILD_TASK_1  "Собирать Task_1 (Волны)"             ON)
option(BUILD_TASK_2  "Собирать Task_2 (Маятник)"           ON)
option(BUILD_TASK_3  "Собирать Task_3 (Гравитация)"        ON)
option(BUILD_NEWTASK "Собирать NewTask (Молекулярная дин.)" ON)

if(BUILD_TASK_0)
    add_subdirectory(Task_0)
endif()
if(BUILD_TASK_1)
    add_subdirectory(Task_1)
endif()
if(BUILD_TASK_2)
    add_subdirectory(Task_2)
endif()
if(BUILD_TASK_3)
    add_subdirectory(Task_3)
endif()
if(BUILD_NEWTASK)
    add_subdirectory(NewTask)
endif()
