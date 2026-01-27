#!/bin/bash
# quick-build.sh - простой скрипт сборки
# Положите в ~/my_code/wrapper_nx/

cd /home/ev/my_code/wrapper_nx

# Проверяем Docker
if ! docker info &> /dev/null; then
    echo "Ошибка: проверьте Docker (sudo docker info)"
    exit 1
fi

echo "Сборка wrapper_nx..."
docker run --rm \
  -v "$(pwd):/project" \
  -w /project \
  devkitpro/devkita64 \
  make

echo "Готово! Проверьте файлы:"
ls -la *.nro *.nsp *.elf 2>/dev/null || echo "Бинарные файлы не найдены"