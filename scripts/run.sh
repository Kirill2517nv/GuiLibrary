#!/usr/bin/env bash
set -euo pipefail

target="${1:-Task_1}"
build_type="${BUILD_TYPE:-Release}"
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_dir="$(cd "$script_dir/.." && pwd)"

if [[ ! -f "$project_dir/build/CMakeCache.txt" ]]; then
    bash "$script_dir/bootstrap.sh" "$target"
else
    cmake --build "$project_dir/build" --config "$build_type" --target "$target" --parallel
fi

candidates=(
    "$project_dir/$target/$target"
    "$project_dir/$target/$target.exe"
    "$project_dir/$target/$build_type/$target"
    "$project_dir/$target/$build_type/$target.exe"
    "$project_dir/build/$target/$target"
    "$project_dir/build/$target/$target.exe"
    "$project_dir/build/$target/$build_type/$target"
    "$project_dir/build/$target/$build_type/$target.exe"
)

for executable in "${candidates[@]}"; do
    if [[ -f "$executable" ]]; then
        exec "$executable"
    fi
done

echo "Ошибка: исполняемый файл $target не найден после успешной сборки." >&2
exit 1
