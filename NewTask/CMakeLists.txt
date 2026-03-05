# Минимальная сборка задания NewTask (молекулярная динамика)
cmake_minimum_required(VERSION 3.16)
project(NewTask CXX)

add_executable(NewTask main.cpp)

# Папка вывода артефактов: рядом с CMakeLists.txt
set_target_properties(NewTask PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Линкуем с основной библиотекой GUI
target_link_libraries(NewTask gui_library)
