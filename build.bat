@echo off
rem Генерирует решение Visual Studio в build\ и открывает его.
rem
rem CMake берётся из самой студии, ставить его отдельно не нужно. Нужен только
rem компонент "Инструменты CMake для C++ для Windows" в установщике VS.
rem
rem Файл в кодировке CP866 - это кодировка консоли Windows. Команды chcp здесь
rem быть не должно: смена кодировки посреди .bat сбивает cmd с позиции чтения
rem файла, и он продолжает разбор с середины строки.
setlocal
cd /d "%~dp0"

rem Без сабмодулей external\ пуст, и cmake падает с невнятной ошибкой про
rem отсутствующий CMakeLists в external/glfw. Проверяем заранее.
if not exist "external\imgui\imgui.h" (
    echo.
    echo   ОШИБКА: папка external\imgui пуста.
    echo   Проект скачан без сабмодулей. Нужен архив целиком либо команда:
    echo       git submodule update --init --recursive
    echo.
    pause
    exit /b 1
)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo.
    echo   ОШИБКА: не найден vswhere.exe, Visual Studio не установлена.
    echo.
    pause
    exit /b 1
)

rem -latest: если студий несколько, берётся самая новая.
rem -requires: студия без компонентов C++ нам не подходит.
set "VSREQ=Microsoft.VisualStudio.Component.VC.Tools.x86.x64"
for /f "usebackq tokens=*" %%s in (`"%VSWHERE%" -latest -products * -requires %VSREQ% -property installationPath`) do set "VSPATH=%%s"
for /f "usebackq tokens=1 delims=." %%s in (`"%VSWHERE%" -latest -products * -requires %VSREQ% -property installationVersion`) do set "VSMAJOR=%%s"

if not defined VSPATH (
    echo.
    echo   ОШИБКА: Visual Studio есть, но без поддержки C++.
    echo   Установщик VS -^> Изменить -^> "Разработка классических приложений на C++".
    echo.
    pause
    exit /b 1
)

rem Имя генератора в CMake жёстко привязано к версии студии.
if "%VSMAJOR%"=="16" set "GEN=Visual Studio 16 2019"
if "%VSMAJOR%"=="17" set "GEN=Visual Studio 17 2022"
if "%VSMAJOR%"=="18" set "GEN=Visual Studio 18 2026"
if not defined GEN (
    echo.
    echo   ОШИБКА: неизвестная версия Visual Studio ^(%VSMAJOR%^).
    echo   Допишите её в build.bat рядом с остальными.
    echo.
    pause
    exit /b 1
)

set "CMAKE=%VSPATH%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not exist "%CMAKE%" set "CMAKE=cmake.exe"

echo.
echo   Студия:  %GEN%
echo   Решение: %CD%\build
echo.

rem Чужой кеш в build\ cmake не перенастраивает, а падает с ошибкой: и при смене
rem версии студии, и если папку создали без -A x64. Проще стереть и собрать заново.
if exist "build\CMakeCache.txt" (
    findstr /c:"CMAKE_GENERATOR:INTERNAL=%GEN%" "build\CMakeCache.txt" >nul || (
        echo   Каталог build\ собран другой студией, пересоздаю...
        rmdir /s /q build
    )
)
if exist "build\CMakeCache.txt" (
    findstr /c:"CMAKE_GENERATOR_PLATFORM:INTERNAL=x64" "build\CMakeCache.txt" >nul || (
        echo   Каталог build\ собран под другую платформу, пересоздаю...
        rmdir /s /q build
    )
)

"%CMAKE%" -S . -B build -G "%GEN%" -A x64
if errorlevel 1 (
    echo.
    echo   ОШИБКА: конфигурация не удалась, решение не создано.
    echo.
    pause
    exit /b 1
)

rem VS 2026 генерирует решение в новом формате .slnx, версии до неё - .sln.
for %%s in ("build\GUI_Library.sln" "build\GUI_Library.slnx") do (
    if exist "%%~s" (
        echo.
        echo   Открываю %%~nxs ...
        start "" "%VSPATH%\Common7\IDE\devenv.exe" "%%~fs"
        exit /b 0
    )
)

echo   ОШИБКА: решение не найдено в build\.
pause
exit /b 1
