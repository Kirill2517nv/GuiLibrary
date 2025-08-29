# Минимальная сборка задания 2
cmake_minimum_required(VERSION 3.16)
project(Task_2 CXX)

add_executable(Task_2 main.cpp)

set_target_properties(Task_2 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Task_2 gui_library)
