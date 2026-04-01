# Минимальная сборка задания 2
cmake_minimum_required(VERSION 3.16)
project(Task_2 CXX)

add_executable(Task_2 main.cpp)

set_target_properties(Task_2 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(Task_2 gui_library)

if(EMSCRIPTEN)
    set_target_properties(Task_2 PROPERTIES SUFFIX ".html")
    target_link_options(Task_2 PRIVATE
        -sUSE_GLFW=3
        -sWASM=1
        -sALLOW_MEMORY_GROWTH=1
        -sMIN_WEBGL_VERSION=2
        -sMAX_WEBGL_VERSION=2
        -sFULL_ES3=1
        --shell-file=${CMAKE_SOURCE_DIR}/web/shell.html
    )
endif()
