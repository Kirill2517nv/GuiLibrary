cmake_minimum_required(VERSION 3.16)
project(GUI_Library)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Настройки для Windows
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Поиск пакетов
find_package(OpenGL REQUIRED)

# Включаем внешние библиотеки как submodules
add_subdirectory(external/glfw)
add_subdirectory(external/imgui)
add_subdirectory(external/implot)

# Создаем основную библиотеку
add_library(gui_library
    src/gui_library.cpp
)

# Подключаем заголовочные файлы
target_include_directories(gui_library PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/external/implot
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include
)

# Линкуем библиотеки
target_link_libraries(gui_library
    imgui
    implot
    glfw
    OpenGL::GL
)

# Создаем исполняемые файлы с примерами
add_executable(simple_example
    examples/simple_example.cpp
)
target_link_libraries(simple_example gui_library)

add_executable(animation_example
    examples/animation_example.cpp
)
target_link_libraries(animation_example gui_library)

add_executable(school_example
    examples/school_example.cpp
)
target_link_libraries(school_example gui_library)

add_executable(docking_test
    examples/docking_test.cpp
)
target_link_libraries(docking_test gui_library)

# Копируем ресурсы (только для simple_example, если папка существует)
if(WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/resources")
    add_custom_command(TARGET simple_example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:simple_example>/resources
    )
endif()

# Подключаем задания Task_1..Task_3
add_subdirectory(Task_1)
add_subdirectory(Task_2)
add_subdirectory(Task_3)
