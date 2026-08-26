add_executable(Task_3 main.cpp)
set_target_properties(Task_3 PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Task_3 gui_library)
configure_wasm_task(Task_3)
