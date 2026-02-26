# GUI Library для численного моделирования

Простая графическая библиотека на C++. Использует ImGui (ветка `docking`) и ImPlot для создания интерактивных графиков и параметров. Сборка осуществляется через git submodules.

## Структура проекта

```
GuiLibrary/
├── CMakeLists.txt          # Основной файл сборки
├── include/
│   └── gui_library.h       # Заголовочный файл библиотеки
├── src/
│   └── gui_library.cpp     # Реализация библиотеки
├── Task_0/
│   ├── CMakeLists.txt  
│   └── main.cpp # Пример с анимацией
├── external/
│   ├── glfw/               # Сабмодуль GLFW
│   ├── imgui/              # Сабмодуль ImGui (ветка docking)
│   └── implot/             # Сабмодуль ImPlot         
├── README.md   
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
# Инициализация и обновление сабмодулей согласно .gitmodules
git submodule update --init --recursive
```

### 4) Конфигурация CMake (Visual Studio 2022, x64)

```powershell
mkdir build 
cd build
cmake ..
```

### 5) Запуск примеров

```powershell
build\Task_0\Release\Task_0.exe
build\Task_1\Release\Task_1.exe
build\Task_2\Release\Task_2.exe
build\Task_3\Release\Task_3.exe
```

## Требования

- C++17
- CMake 3.16+
- OpenGL 3.3+
- GLFW (сабмодуль)
- ImGui (ветка `docking`, сабмодуль)
- ImPlot (сабмодуль)

## API библиотеки

### Инициализация

```cpp
#include "gui_library.h"

// Инициализация
if (!init_gui_library("My project")) {
    return -1;
}
```

### Добавление параметров

```cpp
// Числовой параметр
add_float_param("mass", 1.0f, 0.1f, 10.0f, 0.1f);

// Целочисленный параметр
add_int_param("steps", 100, 10, 1000, 10);

// Логический параметр
add_bool_param("show_grid", true);

// Строковый параметр
add_string_param("title", "My graph");
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
create_plot("Graph", scale);

// Добавление данных
std::vector<float> x = {1, 2, 3, 4, 5};
std::vector<float> y = {1, 4, 9, 16, 25};

// Добавление точки на график:
add_plot_scatter("Graph", x[0], y[0], "y = x²", RED); // В скобках указываются по порядку: название графика, значение по оси X, значение по оси Y, название отрисованного объекта, цвет объекта на графике

// Добавление линии из точек на график:
add_plot_scatterline("Graph", x, y, "y = x²", BLUE); 

// Координаты точек (0, 0) и (5, 7) 
std::vector<float> coordX = {0.0f, 5.0f};
std::vector<float> coordY = {0.0f, 7.0f};
// Добавление отрезка на график:
add_plot_line("Graph", coordX, coordY, "section", WHITE); 

// Очистка графика
clear_plot("Graph");
```

### Функция расчета 

```cpp
void calc_function() {
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
    clear_plot("Graph");
    add_plot_data("Graph", x_values, y_values, "Result", RED);
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

### Как добавить новый параметр:

1. **Определите тип параметра:**
   - `add_float_param()` - для чисел с плавающей точкой
   - `add_int_param()` - для целых чисел
   - `add_bool_param()` - для да/нет
   - `add_string_param()` - для текста

2. **Укажите имя и название переменной:**
   ```cpp
   add_float_param("speed", speed);
   ```

3. **Получите значение в функции расчета:**
   ```cpp
   float speed = get_float_param("speed");
   ```

### Как написать функцию расчета:

1. **Создайте функцию или лямбда-функцию:**
   ```cpp
   void my_calc() {
       // Ваш код здесь
   };

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
   clear_plot("Graph");
   add_plot_scatterline("Graph", x_values, y_values, "Sin", BLUE);
   ```

6. **Установите функцию:**
   ```cpp
   set_calculation_function(my_calc);
   ```
