#!/bin/bash
# Запуск nxlink: отправка .nro на Switch и приём логов.
# Использование: ./nxlink_debug.sh 192.168.0.106  (или без IP — поиск по broadcast)
# На Switch: в Homebrew Menu нажмите Y (netloader).

cd "$(dirname "$0")"
NRO="project.nro"
[ -f "$NRO" ] || NRO="$(ls -1 *.nro 2>/dev/null | head -1)"
[ -z "$NRO" ] && { echo "Не найден .nro в $(pwd)"; exit 1; }

if [ -n "$1" ]; then
  docker run --rm -it --network host -v "$(pwd):/project" -w /project \
    devkitpro/devkita64 \
    /opt/devkitpro/tools/bin/nxlink -a "$1" -s "$NRO"
else
  docker run --rm -it --network host -v "$(pwd):/project" -w /project \
    devkitpro/devkita64 \
    /opt/devkitpro/tools/bin/nxlink -s "$NRO"
fi
