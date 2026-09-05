# Заготовка. Скопировали папку под своим именем – замените здесь Template
# на это имя (четыре места) и добавьте add_subdirectory в корневой CMakeLists.
add_executable(Template main.cpp)
set_target_properties(Template PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Template gui_library)
configure_wasm_task(Template)
