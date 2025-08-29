# GUI Library для численного моделирования

Простая графическая библиотека на C++ для школьников, изучающих численное моделирование. Использует ImGui (ветка `docking`) и ImPlot для создания интерактивных графиков и параметров. Сборка осуществляется без vcpkg — через git submodules.

## Особенности

- **Простой API** — минимум ООП, максимум функций
- **Лямбда-функции** для расчетов
- **Докируемые окна** — левая панель с параметрами, правая с графиками
- **Реальное время** — обновление графиков в реальном времени
- **Простота использования** — идеально для школьников

## Структура проекта

```
GuiLibrary/
├── CMakeLists.txt          # Основной файл сборки
├── include/
│   └── gui_library.h       # Заголовочный файл библиотеки
├── src/
│   └── gui_library.cpp     # Реализация библиотеки
├── examples/
│   ├── simple_example.cpp  # Простой пример (синусоида)
│   ├── animation_example.cpp # Пример с анимацией
│   ├── school_example.cpp  # Пример для школьников
│   └── docking_test.cpp    # Тест докинга
├── external/
│   ├── glfw/               # Сабмодуль GLFW
│   ├── imgui/              # Сабмодуль ImGui (ветка docking)
│   └── implot/             # Сабмодуль ImPlot
└── resources/              # Ресурсы
```

## Быстрый старт (Windows 10/11, Visual Studio 2022)

### 1) Требования

- Visual Studio 2022 (рабочая нагрузка «Разработка классических приложений на C++»)
- CMake 3.16+
- Git

Проверьте, что инструменты доступны в PATH:

```powershell
cmake --version
git --version
```

### 2) Клонирование репозитория

```powershell
cd C:\GitHub
git clone <URL_ВАШЕГО_РЕПОЗИТОРИЯ> GuiLibrary
cd GuiLibrary
```

### 3) Инициализация сабмодулей (без vcpkg)

Проект использует сабмодули `GLFW`, `ImGui`, `ImPlot`. Для `ImGui` автоматически используется ветка `docking` (зафиксировано в `.gitmodules`). Выполните:

```powershell
# Синхронизация настроек сабмодулей (на случай изменений)
git submodule sync --recursive

# Инициализация и обновление сабмодулей согласно .gitmodules
git submodule update --init --recursive

# Явная установка ветки docking для ImGui и получение последних изменений
git -C external/imgui fetch origin docking
git -C external/imgui checkout docking
git -C external/imgui pull --ff-only origin docking
```

Проверить активную ветку ImGui:

```powershell
git -C external/imgui status
```

Ожидаемо: `On branch docking`.

### 4) Конфигурация CMake (Visual Studio 2022, x64)

```powershell
cmake -B build -S . -G "Visual Studio 17 2022" -A x64
```

Если CMake нашёл OpenGL (`opengl32`) и добавил цели `glfw`, `imgui`, `implot`, конфигурация прошла успешно.

### 5) Сборка

```powershell
cmake --build build --config Release
```

После сборки исполняемые файлы появятся в `build\Release\`.

### 6) Запуск примеров

```powershell
.\n+build\Release\simple_example.exe
build\Release\animation_example.exe
build\Release\school_example.exe
build\Release\docking_test.exe
```

## Требования

- C++17
- CMake 3.16+
- OpenGL 3.3+
- GLFW (сабмодуль)
- ImGui (ветка `docking`, сабмодуль)
- ImPlot (сабмодуль)

## Устранение проблем

- «CMake не видит OpenGL»: убедитесь, что система — Windows 10/11; обычно находится как `opengl32` автоматически.
- «Сабмодули пустые или не скачались»: повторите команды инициализации:
  ```powershell
  git submodule sync --recursive
  git submodule update --init --recursive
  git -C external/imgui fetch origin docking
  git -C external/imgui checkout docking
  git -C external/imgui pull --ff-only origin docking
  ```
- «Ошибки линковки GLFW/ImGui»: убедитесь, что `external/glfw`, `external/imgui`, `external/implot` содержат исходники, а CMake конфигурация прошла без ошибок.

## API библиотеки

### Инициализация

```cpp
#include "gui_library.h"

// Инициализация
if (!init_gui_library("Мой проект")) {
    return -1;
}
```

### Добавление параметров

```cpp
// Числовой параметр
add_float_param("mass", "Масса", 1.0f, 0.1f, 10.0f, 0.1f);

// Целочисленный параметр
add_int_param("steps", "Количество шагов", 100, 10, 1000, 10);

// Логический параметр
add_bool_param("show_grid", "Показать сетку", true);

// Строковый параметр
add_string_param("title", "Заголовок", "Мой график");
```

### Получение значений параметров

```cpp
float mass = get_float_param("mass");
int steps = get_int_param("steps");
bool show_grid = get_bool_param("show_grid");
std::string title = get_string_param("title");
```

### Работа с графиками

```cpp
// Создание графика
create_plot("Мой график", "Заголовок");

// Добавление данных
std::vector<float> x = {1, 2, 3, 4, 5};
std::vector<float> y = {1, 4, 9, 16, 25};
add_plot_data("Мой график", x, y, "y = x²");

// Очистка графика
clear_plot("Мой график");
```

### Функция расчета (лямбда)

```cpp
auto calc_function = []() {
    // Получаем параметры
    float mass = get_float_param("mass");
    int steps = get_int_param("steps");
    
    // Создаем данные
    std::vector<float> x_values(steps);
    std::vector<float> y_values(steps);
    
    // Заполняем данные
    for (int i = 0; i < steps; ++i) {
        float x = (float)i / steps * 10.0f;
        x_values[i] = x;
        y_values[i] = mass * x * x; // Ваша формула
    }
    
    // Обновляем график
    clear_plot("Мой график");
    add_plot_data("Мой график", x_values, y_values, "Результат");
};

// Устанавливаем функцию расчета
set_calculation_function(calc_function);
```

### Основной цикл

```cpp
while (gui_main_loop()) {
    // Функция расчета вызывается автоматически
    sleep_ms(16); // Для анимации
}

// Завершение
shutdown_gui_library();
```

## Примеры использования

### 1. Простая синусоида

См. `examples/simple_example.cpp` - демонстрирует:
- Добавление параметров
- Создание графика
- Использование лямбда-функции для расчетов

### 2. Анимированная волна

См. `examples/animation_example.cpp` - демонстрирует:
- Анимацию в реальном времени
- Использование времени
- Интерактивные параметры

## Для школьников

### Как добавить новый параметр:

1. **Определите тип параметра:**
   - `add_float_param()` - для чисел с плавающей точкой
   - `add_int_param()` - для целых чисел
   - `add_bool_param()` - для да/нет
   - `add_string_param()` - для текста

2. **Укажите имя и описание:**
   ```cpp
   add_float_param("speed", "Скорость движения", 1.0f);
   ```

3. **Получите значение в функции расчета:**
   ```cpp
   float speed = get_float_param("speed");
   ```

### Как написать функцию расчета:

1. **Создайте лямбда-функцию:**
   ```cpp
   auto my_calc = []() {
       // Ваш код здесь
   };
   ```

2. **Получите параметры:**
   ```cpp
   float param1 = get_float_param("param1");
   ```

3. **Создайте массивы данных:**
   ```cpp
   std::vector<float> x_values(100);
   std::vector<float> y_values(100);
   ```

4. **Заполните данные по вашей формуле:**
   ```cpp
   for (int i = 0; i < 100; ++i) {
       x_values[i] = i * 0.1f;
       y_values[i] = param1 * sin(x_values[i]); // Ваша формула
   }
   ```

5. **Обновите график:**
   ```cpp
   clear_plot("График");
   add_plot_data("График", x_values, y_values, "Мои данные");
   ```

6. **Установите функцию:**
   ```cpp
   set_calculation_function(my_calc);
   ```

## Лицензия

MIT License
