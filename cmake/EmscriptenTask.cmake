# Вспомогательная функция для настройки WASM-сборки задания.
# Использование: configure_wasm_task(Task_N)
function(configure_wasm_task target)
    if(EMSCRIPTEN)
        set_target_properties(${target} PROPERTIES SUFFIX ".html")
        target_link_options(${target} PRIVATE
            -sUSE_GLFW=3
            -sWASM=1
            -sALLOW_MEMORY_GROWTH=1
            -sMIN_WEBGL_VERSION=2
            -sMAX_WEBGL_VERSION=2
            -sFULL_ES3=1
            --shell-file=${CMAKE_SOURCE_DIR}/web/shell.html
        )
    endif()
endfunction()
