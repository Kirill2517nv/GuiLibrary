add_executable(NewTask main.cpp)
set_target_properties(NewTask PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(NewTask gui_library)
configure_wasm_task(NewTask)
