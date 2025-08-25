# Инструкции по сборке GUI Library

## Требования

- **Windows 10/11**
- **Visual Studio 2022** или **MinGW-w64**
- **CMake 3.16+**
- **Git**

## Способ 1: Автоматическая настройка (рекомендуется)

### 1. Запуск скрипта настройки

Откройте PowerShell от имени администратора и выполните:

```powershell
# Перейдите в папку проекта
cd C:\Users\Kirill\CursorProjects\GUI_Library

# Запустите скрипт настройки
.\setup_dependencies.ps1
```

Скрипт автоматически:
- Загрузит ImGui и ImPlot
- Создаст необходимые CMakeLists.txt файлы
- Настроит структуру проекта

### 2. Сборка проекта

```powershell
# Создайте папку для сборки
mkdir build
cd build

# Настройте проект
cmake .. -G "Visual Studio 17 2022" -A x64

# Соберите проект
cmake --build . --config Release
```

### 3. Запуск примеров

```powershell
# Простой пример
.\Release\simple_example.exe

# Пример с анимацией
.\Release\animation_example.exe

# Пример для школьников
.\Release\school_example.exe
```

## Способ 2: Ручная настройка

### 1. Установка vcpkg (если не установлен)

```powershell
# Клонируйте vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# Установите vcpkg
.\bootstrap-vcpkg.bat

# Добавьте в PATH (замените путь на ваш)
$env:PATH += ";C:\vcpkg"
```

### 2. Установка зависимостей

```powershell
# Установите необходимые пакеты
vcpkg install glfw3:x64-windows
vcpkg install imgui:x64-windows
vcpkg install implot:x64-windows

# Интегрируйте с Visual Studio
vcpkg integrate install
```

### 3. Настройка CMake

Создайте файл `CMakePresets.json` в корне проекта:

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "default",
      "binaryDir": "${sourceDir}/build",
      "cacheVariables": {
        "CMAKE_TOOLCHAIN_FILE": "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
      }
    }
  ]
}
```

### 4. Сборка

```powershell
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## Способ 3: Использование MinGW-w64

### 1. Установка MinGW-w64

Скачайте и установите MinGW-w64 с [официального сайта](https://www.mingw-w64.org/).

### 2. Установка зависимостей

```bash
# Установите пакеты через MSYS2 (если используете)
pacman -S mingw-w64-x86_64-glfw
pacman -S mingw-w64-x86_64-opengl-headers
```

### 3. Сборка

```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
```

## Устранение проблем

### Ошибка "GLFW not found"

```powershell
# Установите GLFW через vcpkg
vcpkg install glfw3:x64-windows
```

### Ошибка "OpenGL not found"

```powershell
# Установите OpenGL заголовки
vcpkg install opengl:x64-windows
```

### Ошибка компиляции ImGui

Убедитесь, что ImGui и ImPlot загружены в папку `external/`:

```powershell
# Проверьте структуру
ls external/
# Должно быть:
# - imgui/
# - implot/
```

### Ошибка линковки

Проверьте, что все библиотеки найдены:

```powershell
# Пересоберите проект
rm -rf build
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## Проверка работы

После успешной сборки запустите примеры:

1. **simple_example.exe** - простая синусоида
2. **animation_example.exe** - анимированная волна  
3. **school_example.exe** - пример для школьников

Вы должны увидеть окно с:
- Левая панель: параметры для настройки
- Правая панель: график, обновляющийся в реальном времени

## Создание своего проекта

1. Скопируйте один из примеров
2. Измените функцию расчета в лямбда-функции
3. Добавьте свои параметры
4. Соберите и запустите

## Поддержка

При возникновении проблем:
1. Проверьте версии всех компонентов
2. Убедитесь, что все зависимости установлены
3. Проверьте логи CMake
4. Попробуйте пересобрать проект с нуля
