# Сборка WebAssembly-версии симуляторов

Инструкция по компиляции заданий в WebAssembly для запуска в браузере.

## Зависимости

| Инструмент | Зачем нужен | Установка |
|---|---|---|
| **Emscripten SDK (emsdk)** | Компилятор C++ → WebAssembly | Уже установлен в `external/emsdk/` |
| **CMake 3.16+** | Система сборки | https://cmake.org |
| **Ninja** | Генератор сборки (нужен для Emscripten) | `pip install ninja` |
| **Python 3** | Нужен для emsdk и HTTP-сервера | https://python.org |

> Нативные зависимости (Visual Studio, OpenGL SDK) для веб-сборки **не нужны**.
> Emscripten предоставляет свои реализации GLFW и OpenGL ES.

## Быстрый старт (Windows, cmd.exe)

### 1. Установка Emscripten (один раз)

```bat
git clone https://github.com/emscripten-core/emsdk.git external/emsdk
cd external\emsdk
python emsdk.py install latest
python emsdk.py activate latest
cd ..\..

pip install ninja
```

### 2. Конфигурация (один раз на директорию build-web)

```bat
external\emsdk\emsdk_env.bat
external\emsdk\upstream\emscripten\emcmake.bat cmake -B build-web -S . -G Ninja
```

### 3. Сборка

```bat
REM Собрать одно задание:
external\emsdk\upstream\emscripten\emmake.bat cmake --build build-web --target Task_2

REM Собрать все задания:
external\emsdk\upstream\emscripten\emmake.bat cmake --build build-web
```

Результат появляется рядом с исходниками задания:
```
Task_2/Task_2.html   ← открыть в браузере
Task_2/Task_2.js     ← Emscripten JS runtime
Task_2/Task_2.wasm   ← скомпилированный C++ код
```

### 4. Запуск

```bat
cd Task_2
python -m http.server 8080
REM Открыть: http://localhost:8080/Task_2.html
```

> **Важно:** WASM нельзя открыть через `file://` — браузер блокирует это из соображений
> безопасности. Нужен любой HTTP-сервер.

## Добавление нового задания с поддержкой WASM

После рефакторинга все задания поддерживают WASM из коробки.
Ничего дополнительно добавлять в `main.cpp` и `CMakeLists.txt` **не нужно**.

### main.cpp

Используй `run_gui_library()` вместо ручного цикла — она сама выбирает стратегию:

```cpp
int main() {
    if (!init_gui_library("Название")) return -1;

    // ... добавление параметров и графиков ...

    set_calculation_function(calculation_function);
    run_gui_library();   // работает и нативно, и в WASM
    return 0;
}
```

### CMakeLists.txt задания

```cmake
add_executable(Task_N main.cpp)
set_target_properties(Task_N PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Task_N gui_library)
configure_wasm_task(Task_N)   // добавляет WASM-флаги автоматически
```

Функция `configure_wasm_task` определена в `cmake/EmscriptenTask.cmake` и включается
в корневом `CMakeLists.txt`. В нативной сборке она ничего не делает.

### Добавить задание в корневой CMakeLists.txt

```cmake
option(BUILD_TASK_N "Описание" ON)
if(BUILD_TASK_N)
    add_subdirectory(Task_N)
endif()
```

## Деплой для студентов (GitHub Pages)

```bash
mkdir docs
cp Task_2/Task_2.{html,js,wasm} docs/
```
В настройках репо → Settings → Pages → Source: `main` branch, `/docs` folder.

Студенты открывают: `https://username.github.io/repo/Task_2.html`

## Архитектурные детали

### Почему run_gui_library() вместо ручного цикла

В браузере нельзя блокировать главный поток бесконечным `while`. Вместо этого
Emscripten использует `requestAnimationFrame` (~60 FPS). `run_gui_library()` скрывает
эту разницу внутри библиотеки:

```cpp
void run_gui_library() {
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(emscripten_loop_step, 0, 1);
#else
    while (gui_main_loop()) { sleep_ms(16); }
    shutdown_gui_library();
#endif
}
```

### Почему ImGui нужны 32-bit индексы

Docking + несколько ImPlot-графиков создают много вершин. По умолчанию ImGui
использует 16-bit индексы (лимит 65536). В корневом CMakeLists.txt включены 32-bit:

```cmake
add_compile_definitions(ImDrawIdx=unsigned\ int)
```

### Флаги Emscripten (из cmake/EmscriptenTask.cmake)

| Флаг | Назначение |
|------|-----------|
| `-sUSE_GLFW=3` | Встроенная реализация GLFW3 (не из submodules) |
| `-sWASM=1` | Генерировать .wasm |
| `-sALLOW_MEMORY_GROWTH=1` | Динамическое выделение (для `std::vector` и т.д.) |
| `-sMIN/MAX_WEBGL_VERSION=2` | WebGL 2.0 = OpenGL ES 3.0 |
| `-sFULL_ES3=1` | Полная эмуляция OpenGL ES 3.0 |
| `--shell-file` | Наш HTML-шаблон вместо стандартного Emscripten |

### Известные нюансы

**Canvas resize:** В `gui_main_loop()` каждый кадр размер canvas подгоняется под
viewport через `glfwSetWindowSize()`. Именно `glfwSetWindowSize`, а не
`emscripten_set_canvas_element_size` — последняя не обновляет внутреннее состояние GLFW,
и `glViewport` ставится неправильно.

**sleep_ms:** В Emscripten является no-op — скорость кадров контролирует браузер.

**ViewportsEnable:** Отключён для WASM — ImGui Viewports создают дополнительные окна ОС,
в браузере это невозможно.
