add_executable(Task_2 main.cpp)
set_target_properties(Task_2 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Task_2 gui_library)
configure_wasm_task(Task_2)
