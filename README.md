# GUI Library для численного моделирования

Простая графическая библиотека на C++ для школьников, изучающих численное моделирование. Использует ImGui и ImPlot для создания интерактивных графиков и параметров.

## Особенности

- **Простой API** - минимум ООП, максимум функций
- **Лямбда-функции** для расчетов
- **Докируемые окна** - левая панель с параметрами, правая с графиками
- **Реальное время** - обновление графиков в реальном времени
- **Простота использования** - идеально для школьников

## Структура проекта

```
GUI_Library/
├── CMakeLists.txt          # Основной файл сборки
├── include/
│   └── gui_library.h       # Заголовочный файл библиотеки
├── src/
│   └── gui_library.cpp     # Реализация библиотеки
├── examples/
│   ├── simple_example.cpp  # Простой пример (синусоида)
│   └── animation_example.cpp # Пример с анимацией
├── external/               # Внешние библиотеки (ImGui, ImPlot)
└── resources/              # Ресурсы
```

## Быстрый старт

### 1. Установка зависимостей

Для Windows с vcpkg:
```bash
vcpkg install glfw3 imgui implot
```

### 2. Сборка проекта

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### 3. Запуск примера

```bash
./example
```

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

## Требования

- C++17
- CMake 3.16+
- OpenGL 3.3+
- GLFW3
- ImGui
- ImPlot

## Лицензия

MIT License
