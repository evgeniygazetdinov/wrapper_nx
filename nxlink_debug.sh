#!/bin/bash
# Запуск nxlink: отправка .nro на Switch и приём логов.
# Использование: ./nxlink_debug.sh 192.168.0.106  (или без IP — поиск по broadcast)
# На Switch: в Homebrew Menu нажмите Y (netloader).
#
# Если после "Sending project.nro" зависает: nxlink ждёт, пока NRO запустится и
# подключится обратно для вывода логов. Если NRO падает до подключения — nxlink
# висит. Вариант: запуск без отправки — сначала ./nxlink_debug.sh IP (без -s),
# потом на Switch запусти NRO с SD (sdmc:/switch/project.nro) — тогда логи появятся,
# если краш после инита nxlink.

cd "$(dirname "$0")"
NRO="project.nro"
[ -f "$NRO" ] || NRO="$(ls -1 *.nro 2>/dev/null | head -1)"
[ -z "$NRO" ] && { echo "Не найден .nro в $(pwd)"; exit 1; }

SIZE_B=$(stat -c%s "$NRO" 2>/dev/null || stat -f%z "$NRO" 2>/dev/null)
SIZE_MB=$((SIZE_B / 1024 / 1024))
echo "NRO: $NRO ($SIZE_MB MB). Отправка по Wi‑Fi может занять 1–2 мин."
echo "На Switch: сначала нажми Y (netloader) в Homebrew Menu, потом запускай скрипт."
echo "Если после «Sending» нет прогресса (X sent) 2+ мин — сеть или netloader."
echo ""

# Режим listen: ./nxlink_debug.sh <IP> listen (nxlink требует nrofile в конце)
if [ "$2" = "listen" ] || [ "$1" = "listen" ]; then
  IP="${1}"
  [ "$1" = "listen" ] && IP="${2}"
  [ -z "$IP" ] && { echo "Укажи IP: $0 <IP> listen"; exit 1; }
  echo "Режим listen: отправка $NRO, сервер логов (на Switch: Y в Homebrew Menu)."
  docker run --rm -it --network host -v "$(pwd):/project" -w /project \
    devkitpro/devkita64 \
    /opt/devkitpro/tools/bin/nxlink "$NRO" -a "$IP" -s
  exit 0
fi

if [ -n "$1" ]; then
  docker run --rm -it --network host -v "$(pwd):/project" -w /project \
    devkitpro/devkita64 \
    /opt/devkitpro/tools/bin/nxlink -a "$1" -s "$NRO"
else
  docker run --rm -it --network host -v "$(pwd):/project" -w /project \
    devkitpro/devkita64 \
    /opt/devkitpro/tools/bin/nxlink -s "$NRO"
fi