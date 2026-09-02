# GUI Library для численного моделирования

Учебная C++17-библиотека поверх ImGui (ветка `docking`) и ImPlot. Она позволяет
добавлять параметры и строить графики, не работая напрямую с окном, OpenGL и
памятью графического движка.

## Быстрый старт

Требуются Git, CMake 3.16+, компилятор C++17 и OpenGL. В Windows рекомендуется
Visual Studio 2022 с workload «Разработка классических приложений на C++» и
Git Bash.

Обычное клонирование и сборка `Task_1`:

```bash
git clone --recurse-submodules https://github.com/Kirill2517nv/GuiLibrary.git
cd GuiLibrary
bash scripts/bootstrap.sh Task_1
bash scripts/run.sh Task_1
```

Если репозиторий уже клонирован без зависимостей, `bootstrap.sh` сам выполнит
`git submodule update --init --recursive`. Для другой задачи передайте `Task_2`,
`Task_3` или `NewTask`.

Для Debug-сборки:

```bash
BUILD_TYPE=Debug bash scripts/bootstrap.sh Task_1
BUILD_TYPE=Debug bash scripts/run.sh Task_1
```

## VS Code

Откройте корень репозитория и установите предложенные расширения CMake Tools и
C/C++. После автоматической конфигурации:

- `Ctrl+Shift+B` собирает `Task_1`;
- конфигурация `Run Task_1 (Windows)` или `Run Task_1 (Linux)` запускает её с отладчиком;
- другую цель можно выбрать командой `CMake: Set Launch/Debug Target`.

## Минимальный пример с историей

```cpp
#include "gui_library.h"
#include <cmath>

float t = 0.f;

void restart() {
    t = 0.f;
    clear_plot_history("Sin");
}

void calculate() {
    if (get_bool_param("Pause")) return;

    t += 0.15f;
    float y = std::sin(t);

    clear_plot("Sin"); // очищает только объекты текущего кадра
    add_plot_history_point("Sin", t, y, "sin(t)", BLUE, 1.f, 2000);
    add_plot_point("Sin", t, y, "Current value", RED);
}

int main() {
    if (!init_gui_library("Sin")) return 1;

    add_bool_param("Pause", false);
    add_button_param("Restart", restart);
    create_plot("Sin", 0.f, 200.f, -2.f, 2.f, 750, 475);
    set_calculation_function(calculate);
    run_gui_library();
}
```

`add_plot_history_point` хранит кольцевой буфер внутри графика. Функция
`clear_plot_history` удаляет накопленную историю, а `set_plot_scale` меняет
границы уже созданного графика. Старые `DataArray`, `Scale` и перегрузка
`create_plot(name, scale, ...)` пока сохранены для совместимости со старыми
учебными работами.

## Структура

```text
include/gui_library.h   публичный API
src/gui_library.cpp     состояние GUI и реализация
Task_1/main.cpp         основной учебный пример
external/               Git-сабмодули GLFW, ImGui и ImPlot
scripts/                сборка и запуск из терминала
.vscode/                сборка и отладка из VS Code
```

Архитектурный разбор и направление дальнейшего развития находятся в
[`docs/ARCHITECTURE_REVIEW.md`](docs/ARCHITECTURE_REVIEW.md).
