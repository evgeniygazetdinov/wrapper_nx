#!/bin/bash
# Копирует собранный .nro на SD-карту Switch.
#
# Два режима (без DBI — по Wi‑Fi через FTP):
#
#   FTP (не нужен DBI): на Switch запусти FTPD из Homebrew, на ПК укажи IP.
#     SWITCH_IP=192.168.0.106 ./deploy_to_switch.sh
#     ./deploy_to_switch.sh ftp 192.168.0.106
#   Порт по умолчанию 5000 (ftpd). Логин: anonymous, пароль пустой.
#
#   MTP (USB + DBI): консоль по USB, DBI → USB Install → MTP, папка SD смонтирована.
#     ./deploy_to_switch.sh
#     ./deploy_to_switch.sh /path/to/switch
#     SWITCH_SD_PATH="/path/to/switch" ./deploy_to_switch.sh
#   Путь по умолчанию: /run/user/1000/gvfs/mtp:host=-_DBI_.../1: SD Card/switch

cd "$(dirname "$0")"

NRO="project.nro"
[ -f "$NRO" ] || NRO="$(ls -1 *.nro 2>/dev/null | head -1)"
[ -z "$NRO" ] && { echo "Не найден .nro в $(pwd). Сначала собери: make"; exit 1; }

FTP_PORT="${SWITCH_FTP_PORT:-5000}"
REMOTE_PATH="/switch/$(basename "$NRO")"
SWITCH_IP="192.168.0.106"
# Режим FTP: SWITCH_IP задан или первый аргумент "ftp" и второй — IP
if [ "$1" = "ftp" ] && [ -n "$2" ]; then
  SWITCH_IP="$2"
fi
if [ -n "$SWITCH_IP" ]; then
  echo "Отправка по FTP на ${SWITCH_IP}:${FTP_PORT} → ${REMOTE_PATH}"
  if ! curl -sS -T "$NRO" "ftp://${SWITCH_IP}:${FTP_PORT}${REMOTE_PATH}" --user "anonymous:"; then
    echo "Ошибка FTP. Проверь: на Switch запущен FTPD (Homebrew), ПК и Switch в одной Wi‑Fi, IP верный."
    exit 1
  fi
  echo "Готово: $NRO → ftp://${SWITCH_IP}${REMOTE_PATH}"
  exit 0
fi

# Режим MTP (копирование в локальную папку)
if [ -n "$1" ] && [ "$1" != "ftp" ]; then
  DEST="$1"
elif [ -n "$SWITCH_SD_PATH" ]; then
  DEST="$SWITCH_SD_PATH"
else
  DEST="/run/user/1000/gvfs/mtp:host=-_DBI_XJE10002145012/1: SD Card/switch"
fi

[[ "$DEST" != */ ]] && DEST="$DEST/"

if [ ! -d "$DEST" ]; then
  echo "Папка не найдена: $DEST"
  echo "MTP: подключи Switch по USB (DBI → USB Install → MTP)."
  echo "FTP: SWITCH_IP=IP ./deploy_to_switch.sh  (на Switch запусти FTPD)."
  exit 1
fi

if ! cp -v "$NRO" "$DEST"; then
  echo "Ошибка копирования (I/O). Возможные причины:"
  echo "  — NRO открыт на Switch (закрой Homebrew/игру), или SD занята"
  echo "  — большой файл (~7 MB): MTP иногда обрывается — попробуй ещё раз"
  echo "  — соединение по USB пошатнулось — переподключи консоль"
  exit 1
fi
echo "Готово: $NRO → $DEST"
