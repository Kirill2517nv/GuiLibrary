# Минимальная сборка задания 3
cmake_minimum_required(VERSION 3.16)
project(Task_3 CXX)

add_executable(Task_3 main.cpp)

set_target_properties(Task_3 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Task_3 gui_library)
