# Минимальная сборка задания NewProject (молекулярная динамика)
cmake_minimum_required(VERSION 3.16)
project(NewProject CXX)

add_executable(NewProject main.cpp)

# Папка вывода артефактов: рядом с CMakeLists.txt
set_target_properties(NewProject PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Линкуем с основной библиотекой GUI
target_link_libraries(NewProject gui_library)
