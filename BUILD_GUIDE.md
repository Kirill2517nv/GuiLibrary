# Подробная инструкция по сборке GUI Library (без vcpkg)

Эта инструкция описывает сборку проекта на Windows 10/11 с использованием Visual Studio 2022, CMake и Git submodules. Внешние библиотеки `GLFW`, `ImGui` (ветка `docking`) и `ImPlot` подключаются как сабмодули, без vcpkg.

## 📋 Требования

### Системные требования
- Windows 10/11 (x64)
- Visual Studio 2022 (Community достаточно) с нагрузкой «Разработка классических приложений на C++»
- PowerShell (встроен)

### ПО
- CMake 3.16+
- Git

Проверка версий:

```powershell
cmake --version
git --version
```

## 🚀 Пошаговая сборка

### Шаг 1. Клонирование репозитория

```powershell
cd C:\GitHub
git clone <URL_ВАШЕГО_РЕПОЗИТОРИЯ> GuiLibrary
cd GuiLibrary
```

### Шаг 2. Инициализация сабмодулей

В репозитории настроен `.gitmodules` так, чтобы `ImGui` был на ветке `docking`. Выполните:

```powershell
git submodule sync --recursive
git submodule update --init --recursive

# Явно проверим и обновим ветку docking у ImGui
git -C external/imgui fetch origin docking
git -C external/imgui checkout docking
git -C external/imgui pull --ff-only origin docking
```

Проверьте статус:

```powershell
git -C external/imgui status   # Должно быть: On branch docking
```

### Шаг 3. Конфигурация CMake (Visual Studio 2022, x64)

```powershell
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
```

Ожидается, что будут найдены цели `glfw`, `imgui`, `implot` и `OpenGL::GL`.

### Шаг 4. Сборка

```powershell
cmake --build build --config Release
```

Исполняемые файлы появятся в `build\Release\`.

### Шаг 5. Запуск примеров

```powershell
build\Release\simple_example.exe
build\Release\animation_example.exe
build\Release\school_example.exe
build\Release\docking_test.exe
```

## 🔧 Устранение проблем

### CMake не видит OpenGL
- На Windows обычно используется системный `opengl32` и находится автоматически.
- Убедитесь, что целевая платформа x64 и установлены компоненты C++ в Visual Studio.

### Сабмодули пустые/не обновились
```powershell
git submodule sync --recursive
git submodule update --init --recursive
git -C external/imgui fetch origin docking
git -C external/imgui checkout docking
git -C external/imgui pull --ff-only origin docking
```

### Ошибки при сборке GLFW/ImGui
- Не редактируйте файлы CMake в папке сабмодулей, если не уверены. Используйте их штатные `CMakeLists.txt`.
- Проверьте, что в корневом `CMakeLists.txt` добавлены сабмодули:
  - `add_subdirectory(external/glfw)`
  - `add_subdirectory(external/imgui)`
  - `add_subdirectory(external/implot)`

### Как пересобрать «с нуля»
```powershell
Remove-Item -Recurse -Force build  # Удалить папку сборки
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

## 📁 Структура проекта после сборки

```
GuiLibrary/
├── build/
│   └── Release/
│       ├── simple_example.exe
│       ├── animation_example.exe
│       ├── school_example.exe
│       └── docking_test.exe
├── external/
│   ├── glfw/
│   ├── imgui/   # ветка docking
│   └── implot/
├── examples/
├── include/
├── src/
└── CMakeLists.txt
```

## 🎯 Что включает проект
- Примеры: простая синусоида, анимация, учебные графики, тест докинга окон.
- Библиотека: функции для параметров и графиков на основе ImGui + ImPlot.

## 📞 Поддержка
1) Проверьте версии CMake/Git/VS. 2) Выполните шаги строго по порядку. 3) При ошибках пересоберите «с нуля».

## 🎉 Готово
Если примеры запускаются — сборка прошла успешно. Можно начинать использовать библиотеку и писать свои расчёты.
