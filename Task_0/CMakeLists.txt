# Минимальная сборка задания 0
cmake_minimum_required(VERSION 3.16)
project(Task_0 CXX)

add_executable(Task_0 main.cpp)

# Папка вывода артефактов: рядом с CMakeLists.txt
set_target_properties(Task_0 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Линкуем с основной библиотекой GUI
target_link_libraries(Task_0 gui_library)