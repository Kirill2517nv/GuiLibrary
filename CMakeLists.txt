cmake_minimum_required(VERSION 3.16)
project(GUI_Library)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Настройки для Windows
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Поиск пакетов
find_package(OpenGL REQUIRED)


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

target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

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
# GLFW (у тебя как submodule со своим CMakeLists)
# ============================================================
add_subdirectory(external/glfw)

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

# Линкуем библиотеки
target_link_libraries(gui_library
    imgui
    implot
    glfw
    OpenGL::GL
)


# Подключаем задания Task_0..Task_3
add_subdirectory(Task_0)
add_subdirectory(Task_1)
add_subdirectory(Task_2)
add_subdirectory(Task_3)
