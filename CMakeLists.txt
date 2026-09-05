cmake_minimum_required(VERSION 3.16)
project(GUI_Library)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Исходники читаются как UTF-8 на всех компиляторах.
#
# Вторая половина проблемы с кириллицей (первая – шрифт, см.
# src/roboto_medium_font.cpp). GCC и Clang считают исходник UTF-8 по умолчанию, а
# MSVC без этого флага – в системной кодировке: на русской Windows это cp1251, и
# строка "Маятник" превращалась в мусор ещё до того, как её увидит ImGui.
# Спасал только BOM, но он стоял лишь в половине файлов (в Task_0 и Task_1 его
# нет). С флагом кодировка одна на все три компилятора, и BOM больше ни на что
# не влияет.
if(MSVC)
    add_compile_options(/utf-8)
endif()

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
    src/roboto_medium_font.cpp
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
option(BUILD_TASK_0  "Собирать Task_0 (Движение под углом)" ON)
option(BUILD_TASK_1  "Собирать Task_1 (Волны)"             ON)
option(BUILD_TASK_2  "Собирать Task_2 (Маятник)"           ON)
option(BUILD_TASK_3  "Собирать Task_3 (Гравитация)"        ON)
option(BUILD_LBM     "Собирать LBM (течение в пористой среде)" ON)
option(BUILD_MOLDYN  "Собирать MolecularDynamics"              ON)
option(BUILD_TEMPLATE "Собирать Template (заготовка своей задачи)" ON)

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
if(BUILD_LBM)
    add_subdirectory(LBM)
endif()
if(BUILD_MOLDYN)
    add_subdirectory(MolecularDynamics)
endif()
if(BUILD_TEMPLATE)
    add_subdirectory(Template)
endif()

# ============================================================
# Проверки
# ============================================================
# Не в WASM: тест пишет файл и печатает результат в консоль, ему нужен обычный
# запуск. Собирается только по требованию, чтобы не удлинять сборку заданий.
if(NOT EMSCRIPTEN)
    option(BUILD_TESTS "Собирать проверки (tests/)" OFF)
    if(BUILD_TESTS)
        add_executable(test_csv tests/test_csv.cpp)
        target_link_libraries(test_csv gui_library)
    endif()
endif()
