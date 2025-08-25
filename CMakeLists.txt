cmake_minimum_required(VERSION 3.16)
project(GUI_Library)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Настройки для Windows
if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Настройка vcpkg toolchain
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

# Поиск пакетов
find_package(OpenGL REQUIRED)

# Путь к GLFW из vcpkg
set(GLFW_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/include")
set(GLFW_LIBRARY_DIR "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-windows/lib")

# Включаем ImGui и ImPlot
add_subdirectory(external/imgui)
add_subdirectory(external/implot)

# Создаем основную библиотеку
add_library(gui_library
    src/gui_library.cpp
)

# Подключаем заголовочные файлы
target_include_directories(gui_library PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/external/implot/implot
    ${GLFW_INCLUDE_DIR}
)

# Линкуем библиотеки
target_link_libraries(gui_library
    imgui
    implot
    ${GLFW_LIBRARY_DIR}/glfw3dll.lib
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

# Копируем шейдеры и ресурсы (если понадобятся)
if(WIN32)
    add_custom_command(TARGET simple_example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:simple_example>/resources
    )
endif()
