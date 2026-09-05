# Сборка WebAssembly-версии симуляторов

Инструкция по компиляции заданий в WebAssembly для запуска в браузере.

## Зависимости

| Инструмент | Зачем нужен | Установка |
|---|---|---|
| **Emscripten SDK (emsdk)** | Компилятор C++ → WebAssembly | В `external/emsdk/`, ставится вручную – см. ниже. В git не хранится |
| **CMake 3.16+** | Система сборки | https://cmake.org |
| **Ninja** | Генератор сборки (нужен для Emscripten) | `pip install ninja` |
| **Python 3** | Нужен для emsdk и HTTP-сервера | https://python.org |

> Нативные зависимости (Visual Studio, OpenGL SDK) для веб-сборки **не нужны**.
> Emscripten предоставляет свои реализации GLFW и OpenGL ES.

## Быстрый старт

### 1. Установка Emscripten (один раз)

```bash
git clone https://github.com/emscripten-core/emsdk.git external/emsdk
cd external/emsdk
python emsdk.py install latest
python emsdk.py activate latest
cd ../..
pip install ninja
```

`external/emsdk/` в `.gitignore`: это чужой SDK на полтора гигабайта, в репозитории
задач ему не место. У свежего клона его нет – ставить придётся.

### 2. Сборка

```bash
bash scripts/build-web.sh            # все задания
bash scripts/build-web.sh Task_1     # одно
BUILD_TYPE=Debug bash scripts/build-web.sh Task_1
```

Собранное кладётся рядом с исходником: `Task_1/Task_1.{html,js,wasm}`.

По умолчанию **Release** – это важно: с отладочной сборкой `.wasm` весит 1,4 МБ
вместо 890 КБ, потому что тянет отладочную libc++ и проверки. Ученик ждёт
загрузку вдвое дольше без всякой пользы.

Скрипт задаёт `EM_CONFIG` сам и не зовёт `emsdk_env.bat`. Тот запоминает
абсолютный путь на момент `emsdk activate`, и после переноса папки падает с
«Система не может найти указанный путь».

### 3. Запуск локально

```bash
cd Task_1
python -m http.server 8080
# открыть http://localhost:8080/Task_1.html
```

> WASM нельзя открыть через `file://` – браузер это блокирует. Нужен HTTP-сервер.

Выкладка на сайт спецкурса – `docs/spetskurs-deploy.md` в репозитории сайта.

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
