add_executable(Task_1 main.cpp)
set_target_properties(Task_1 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Task_1 gui_library)
configure_wasm_task(Task_1)
