#!/usr/bin/env bash
set -euo pipefail

# Можно запустить из уже клонированного проекта:
#   bash scripts/bootstrap.sh [Task_1]
# или скачать скрипт отдельно и передать URL и каталог:
#   bash bootstrap.sh Task_1 https://github.com/user/GuiLibrary.git GuiLibrary

target="${1:-Task_1}"
repo_url="${2:-https://github.com/Kirill2517nv/GuiLibrary.git}"
destination="${3:-GuiLibrary}"
build_type="${BUILD_TYPE:-Release}"

require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Ошибка: не найдена команда '$1'. Установите её и повторите запуск." >&2
        exit 1
    fi
}

require_command git
require_command cmake

if git rev-parse --show-toplevel >/dev/null 2>&1; then
    project_dir="$(git rev-parse --show-toplevel)"
else
    if [[ -e "$destination" ]]; then
        echo "Ошибка: путь '$destination' уже существует, но не является текущим Git-проектом." >&2
        exit 1
    fi
    git clone --recurse-submodules "$repo_url" "$destination"
    project_dir="$(cd "$destination" && pwd)"
fi

git -C "$project_dir" submodule update --init --recursive

cmake_args=(
    -S "$project_dir"
    -B "$project_dir/build"
    -DCMAKE_BUILD_TYPE="$build_type"
    -DBUILD_TASK_0=OFF
    -DBUILD_TASK_1=OFF
    -DBUILD_TASK_2=OFF
    -DBUILD_TASK_3=OFF
    -DBUILD_TASK_4=OFF
    -DBUILD_NEWTASK=OFF
)

case "$target" in
    Task_0|Task_1|Task_2|Task_3|Task_4) cmake_args+=("-DBUILD_${target^^}=ON") ;;
    NewTask)                            cmake_args+=("-DBUILD_NEWTASK=ON") ;;
    Template)                           cmake_args+=("-DBUILD_TEMPLATE=ON") ;;
    *)
        echo "Ошибка: неизвестная цель '$target'. Используйте Task_0, Task_1, Task_2, Task_3, Task_4, NewTask или Template." >&2
        exit 1
        ;;
esac

cmake "${cmake_args[@]}"
cmake --build "$project_dir/build" --config "$build_type" --target "$target" --parallel

echo "Сборка $target завершена. Запуск: bash scripts/run.sh $target"
