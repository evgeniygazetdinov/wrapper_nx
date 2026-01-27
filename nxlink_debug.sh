#!/bin/bash
# Запуск nxlink через Docker для приёма логов с Switch.
# Использование:
#   ./nxlink_debug.sh              — поиск Switch по broadcast
#   ./nxlink_debug.sh 192.168.1.5  — указать IP Switch
# Перед запуском: на Switch в Homebrew Menu нажмите Y (netloader).

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
