#!/usr/bin/env bash
# Сборка заданий в WebAssembly.
#
#   bash scripts/build-web.sh              все задания
#   bash scripts/build-web.sh Task_1       одно задание
#   BUILD_TYPE=Debug bash scripts/build-web.sh Task_1
#
# Результат кладётся рядом с исходником задания: Task_1/Task_1.{html,js,wasm}.
# Выкладка на сайт – docs/spetskurs-deploy.md.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EMSDK="$ROOT/external/emsdk"
BUILD_DIR="$ROOT/build-web"
BUILD_TYPE="${BUILD_TYPE:-Release}"

if [ ! -d "$EMSDK/upstream/emscripten" ]; then
    echo "Не найден Emscripten в $EMSDK" >&2
    echo "Установка: см. WEB_BUILD.md" >&2
    exit 1
fi

# EM_CONFIG задаём явно, а не через emsdk_env.bat: тот запоминает абсолютный путь
# на момент `emsdk activate`, и после переноса или переименования папки падает с
# «Система не может найти указанный путь». Файл .emscripten лежит внутри самого
# emsdk, поэтому по нему всё находится независимо от того, где папка сейчас.
export EM_CONFIG="$EMSDK/.emscripten"

# На Linux и macOS emsdk кладёт emcmake/emmake без расширения, на Windows –
# только emcmake.bat/emmake.bat. Берём то, что есть.
if [ -f "$EMSDK/upstream/emscripten/emcmake" ]; then
    EMCMAKE="$EMSDK/upstream/emscripten/emcmake"
    EMMAKE="$EMSDK/upstream/emscripten/emmake"
else
    EMCMAKE="$EMSDK/upstream/emscripten/emcmake.bat"
    EMMAKE="$EMSDK/upstream/emscripten/emmake.bat"
fi

# Release обязателен по умолчанию: без него Emscripten собирает с отладочной
# libc++ и проверками, и .wasm выходит 1,4 МБ вместо 890 КБ – ученик ждёт
# загрузку вдвое дольше на ровном месте.
"$EMCMAKE" cmake -B "$BUILD_DIR" -S "$ROOT" -G Ninja -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

if [ $# -gt 0 ]; then
    for target in "$@"; do
        "$EMMAKE" cmake --build "$BUILD_DIR" --target "$target"
    done
else
    "$EMMAKE" cmake --build "$BUILD_DIR"
fi

echo
echo "Готово ($BUILD_TYPE). Собранные файлы:"
find "$ROOT" -maxdepth 2 -name '*.wasm' -newermt '-10 minutes' -printf '  %p (%s байт)\n' 2>/dev/null \
    || find "$ROOT" -maxdepth 2 -name '*.wasm' -exec ls -la {} \;
