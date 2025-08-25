# Скрипт для настройки зависимостей GUI Library
# Запускать от имени администратора

Write-Host "Настройка зависимостей для GUI Library..." -ForegroundColor Green

# Создаем папку external если её нет
if (!(Test-Path "external")) {
    New-Item -ItemType Directory -Path "external"
}

# Переходим в папку external
Set-Location "external"

# Загружаем ImGui
Write-Host "Загрузка ImGui..." -ForegroundColor Yellow
if (!(Test-Path "imgui")) {
    git clone https://github.com/ocornut/imgui.git
} else {
    Write-Host "ImGui уже загружен" -ForegroundColor Cyan
}

# Загружаем ImPlot
Write-Host "Загрузка ImPlot..." -ForegroundColor Yellow
if (!(Test-Path "implot")) {
    git clone https://github.com/epezent/implot.git
} else {
    Write-Host "ImPlot уже загружен" -ForegroundColor Cyan
}

# Создаем CMakeLists.txt для ImGui
Write-Host "Создание CMakeLists.txt для ImGui..." -ForegroundColor Yellow
$imguiCMake = @"
add_library(imgui
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_glfw.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    imgui
    imgui/backends
)

target_link_libraries(imgui glfw OpenGL::GL)
"@

Set-Content -Path "imgui/CMakeLists.txt" -Value $imguiCMake

# Создаем CMakeLists.txt для ImPlot
Write-Host "Создание CMakeLists.txt для ImPlot..." -ForegroundColor Yellow
$implotCMake = @"
add_library(implot
    implot/implot.cpp
    implot/implot_items.cpp
)

target_include_directories(implot PUBLIC
    implot
)

target_link_libraries(implot imgui)
"@

Set-Content -Path "implot/CMakeLists.txt" -Value $implotCMake

# Возвращаемся в корневую папку
Set-Location ".."

Write-Host "Зависимости настроены успешно!" -ForegroundColor Green
Write-Host "Теперь можно собирать проект с помощью CMake" -ForegroundColor Cyan
