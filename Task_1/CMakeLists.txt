# Минимальная сборка задания 1
cmake_minimum_required(VERSION 3.16)
project(Task_1 CXX)

add_executable(Task_1 main.cpp)

# Папка вывода артефактов: рядом с CMakeLists.txt
set_target_properties(Task_1 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Линкуем с основной библиотекой GUI
target_link_libraries(Task_1 gui_library)
