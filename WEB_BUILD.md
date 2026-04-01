# Сборка WebAssembly-версии симуляторов

Инструкция по компиляции заданий (Task_0..Task_3) в WebAssembly для запуска в браузере.

## Зависимости

| Инструмент | Зачем нужен | Установка |
|---|---|---|
| **Emscripten SDK (emsdk)** | Компилятор C++ → WebAssembly | Уже установлен в `external/emsdk/` |
| **CMake 3.16+** | Система сборки | https://cmake.org |
| **Ninja** | Генератор сборки (нужен для Emscripten) | `pip install ninja` |
| **Python 3** | Нужен для emsdk | https://python.org |

> Нативные зависимости (Visual Studio, OpenGL SDK) для веб-сборки **не нужны**.
> Emscripten предоставляет свои реализации GLFW и OpenGL ES.

## Быстрый старт (на примере Task_2)

### 1. Установка Emscripten (один раз)

```bash
# Если emsdk ещё не установлен:
git clone https://github.com/emscripten-core/emsdk.git external/emsdk
cd external/emsdk
python emsdk.py install latest
python emsdk.py activate latest
cd ../..

# Также нужен Ninja:
pip install ninja
```

### 2. Конфигурация и сборка

```bash
# Из корня проекта, в cmd.exe:
external\emsdk\emsdk_env.bat
external\emsdk\upstream\emscripten\emcmake.bat cmake -B build-web -S . -G Ninja
external\emsdk\upstream\emscripten\emmake.bat cmake --build build-web --target Task_2
```

Результат: `Task_2/Task_2.html`, `Task_2/Task_2.js`, `Task_2/Task_2.wasm`

### 3. Запуск

```bash
cd Task_2
python -m http.server 8080
# Открыть http://localhost:8080/Task_2.html
```

> **Важно:** WASM нельзя открыть через `file://`. Нужен HTTP-сервер.

## Как адаптировать новое задание для веба

### Шаг 1: Изменить `main.cpp`

Добавить в начало файла:
```cpp
#ifdef __EMSCRIPTEN__
#include <emscripten.h>

void emscripten_main_loop() {
    gui_main_loop();
}
#endif
```

Заменить главный цикл:
```cpp
// Было:
while (gui_main_loop()) {
    sleep_ms(16);
}

// Стало:
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(emscripten_main_loop, 0, true);
#else
    while (gui_main_loop()) {
        sleep_ms(16);
    }
#endif
```

**Почему:** В браузере нельзя блокировать главный поток бесконечным циклом.
`emscripten_set_main_loop` регистрирует callback, который браузер вызывает
через `requestAnimationFrame` (~60 FPS). Нативная сборка продолжает работать как раньше.

> **Важно:** Нужна именно обычная функция (`void emscripten_main_loop()`),
> а не лямбда. Лямбды с захватом не конвертируются в C-указатель на функцию,
> который ожидает `emscripten_set_main_loop`.

### Шаг 2: Добавить Emscripten-флаги в `CMakeLists.txt` задания

```cmake
if(EMSCRIPTEN)
    set_target_properties(Task_N PROPERTIES SUFFIX ".html")
    target_link_options(Task_N PRIVATE
        -sUSE_GLFW=3
        -sWASM=1
        -sALLOW_MEMORY_GROWTH=1
        -sMIN_WEBGL_VERSION=2
        -sMAX_WEBGL_VERSION=2
        -sFULL_ES3=1
        --shell-file=${CMAKE_SOURCE_DIR}/web/shell.html
    )
endif()
```

Что делают флаги:
- `-sUSE_GLFW=3` — использовать встроенную реализацию GLFW3 (не нашу из submodules)
- `-sWASM=1` — генерировать .wasm (а не asm.js)
- `-sALLOW_MEMORY_GROWTH=1` — динамическое выделение памяти (для `std::vector` и т.д.)
- `-sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2` — WebGL 2.0 = OpenGL ES 3.0
- `-sFULL_ES3=1` — полная эмуляция OpenGL ES 3.0
- `--shell-file` — наш HTML-шаблон вместо стандартного Emscripten

### Шаг 3: Готово

Нативная сборка не затрагивается — все изменения внутри `#ifdef __EMSCRIPTEN__`.

## Изменения в корневом CMakeLists.txt (уже сделаны)

Эти изменения уже внесены и нужны для того чтобы Emscripten-сборка работала:

```cmake
# 32-bit индексы ImGui — без этого переполнение при docking
add_compile_definitions(ImDrawIdx=unsigned\ int)

# OpenGL — не нужен для Emscripten
if(NOT EMSCRIPTEN)
    find_package(OpenGL REQUIRED)
endif()

# GLFW — Emscripten использует свою реализацию
if(NOT EMSCRIPTEN)
    add_subdirectory(external/glfw)
endif()

# imgui — без glfw/OpenGL для Emscripten
if(EMSCRIPTEN)
    target_link_libraries(imgui PUBLIC)
else()
    target_link_libraries(imgui PUBLIC glfw OpenGL::GL)
endif()

# gui_library — без glfw/OpenGL для Emscripten
if(EMSCRIPTEN)
    target_link_libraries(gui_library imgui implot)
else()
    target_link_libraries(gui_library imgui implot glfw OpenGL::GL)
endif()
```

## Изменения в gui_library.cpp (уже сделаны)

### OpenGL ES вместо OpenGL Core Profile
```cpp
#ifdef __EMSCRIPTEN__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
```
**Почему:** Браузер поддерживает только OpenGL ES 3.0 (через WebGL 2.0), а не desktop OpenGL 3.3.

### GLSL версия
```cpp
#ifdef __EMSCRIPTEN__
    ImGui_ImplOpenGL3_Init("#version 300 es");
#else
    ImGui_ImplOpenGL3_Init("#version 330");
#endif
```
**Почему:** WebGL 2.0 использует GLSL ES 3.00, а не GLSL 3.30.

### Viewports отключены
```cpp
#ifndef __EMSCRIPTEN__
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif
```
**Почему:** ImGui Viewports создают дополнительные окна ОС — в браузере это невозможно.

### Автоматический resize canvas под размер браузера
```cpp
#ifdef __EMSCRIPTEN__
EM_JS(int, get_browser_width, (), { return window.innerWidth; });
EM_JS(int, get_browser_height, (), { return window.innerHeight; });
#endif

// В gui_main_loop(), перед glfwPollEvents():
#ifdef __EMSCRIPTEN__
    {
        int w = get_browser_width();
        int h = get_browser_height();
        glfwSetWindowSize(g_window, w, h);
    }
#endif
```
**Почему:** В Emscripten нет системного окна — вместо него canvas элемент в HTML.
Нужно вручную подгонять его под viewport браузера на каждом кадре.

> **Важно:** Использовать именно `glfwSetWindowSize()`, а НЕ `emscripten_set_canvas_element_size()`.
> Последняя функция меняет DOM-элемент, но не обновляет внутреннее состояние GLFW.
> В результате `glfwGetFramebufferSize()` возвращает старый размер,
> `glViewport` ставится неправильно, и рендеринг занимает только часть экрана.

### sleep_ms — no-op в Emscripten
```cpp
void sleep_ms(int milliseconds) {
#ifndef __EMSCRIPTEN__
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
#endif
}
```
**Почему:** В браузере нельзя блокировать поток. Фреймрейт контролирует `requestAnimationFrame`.

## Баги, с которыми мы столкнулись, и их решения

### 1. "Too many vertices in ImDrawList using 16-bit indices"

**Симптом:** Серый экран + exception в консоли.

**Причина:** ImGui по умолчанию использует 16-bit индексы (макс 65536 вершин).
Docking + несколько графиков + ImPlot легко превышают этот лимит.

**Решение:** В корневом `CMakeLists.txt`:
```cmake
add_compile_definitions(ImDrawIdx=unsigned\ int)
```
Это включает 32-bit индексы для всех target'ов (imgui, implot, gui_library, Task_N).

### 2. "Cannot set timing mode for main loop since a main loop does not exist"

**Симптом:** Предупреждение в консоли браузера.

**Причина:** `emscripten_set_main_loop` с C++ лямбдой (lambda) не работает —
функция ожидает указатель на C-функцию типа `void(*)()`.

**Решение:** Вынести callback в обычную функцию:
```cpp
// Плохо:
emscripten_set_main_loop([]() { gui_main_loop(); }, 0, true);

// Хорошо:
void emscripten_main_loop() { gui_main_loop(); }
emscripten_set_main_loop(emscripten_main_loop, 0, true);
```

### 3. Canvas рендерит только часть экрана

**Симптом:** ImGui-контент отображается только в левой части браузера.

**Причина:** `glfwCreateWindow(850, 1200, ...)` создаёт canvas 850×1200.
Если менять размер через `emscripten_set_canvas_element_size()`, GLFW
внутренне по-прежнему считает окно 850×1200, и `glViewport` ставится неправильно.

**Решение:** Использовать `glfwSetWindowSize()` — она обновляет и canvas, и GLFW state.

### 4. Нужен HTTP-сервер

**Симптом:** При открытии .html через `file://` — пустой экран или ошибка CORS.

**Причина:** Браузеры блокируют загрузку .wasm через `file://` из соображений безопасности.

**Решение:** Запустить любой HTTP-сервер:
```bash
python -m http.server 8080
```

## Деплой для студентов (GitHub Pages)

1. Создать папку `docs/` в корне репо
2. Скопировать туда `Task_2.html`, `Task_2.js`, `Task_2.wasm`
3. В настройках GitHub репо → Pages → Source: `main` branch, `/docs` folder
4. Студенты открывают `https://username.github.io/repo/Task_2.html`

Или использовать GitHub Actions для автоматической сборки при каждом push.

## Структура файлов веб-сборки

```
GuiLibrary/
├── web/
│   └── shell.html          ← HTML-шаблон (fullscreen canvas, без Emscripten-шапки)
├── external/
│   └── emsdk/              ← Emscripten SDK (в .gitignore, не коммитить!)
├── build-web/              ← Директория сборки (в .gitignore)
├── Task_2/
│   ├── main.cpp            ← С #ifdef __EMSCRIPTEN__ блоками
│   ├── CMakeLists.txt      ← С if(EMSCRIPTEN) блоком
│   ├── Task_2.html         ← Выходной файл (результат сборки)
│   ├── Task_2.js           ← Emscripten JS runtime
│   └── Task_2.wasm         ← Скомпилированный C++ код
└── src/
    └── gui_library.cpp     ← С Emscripten-совместимыми ifdef'ами
```
