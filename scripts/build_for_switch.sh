#!/bin/bash
# Скрипт для сборки готовой папки Bully для Nintendo Switch
# Использование: ./build_for_switch.sh [выходная_папка] [папка_с_данными] [--zip]

set -e  # Остановить при ошибке

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="./switch_bully_ready"
BULLY_DATA_DIR="./bully_data"
CREATE_ZIP=false

# Обработка параметров
for arg in "$@"; do
    if [ "$arg" = "--zip" ]; then
        CREATE_ZIP=true
    elif [ "$arg" = "--help" ] || [ "$arg" = "-h" ]; then
        echo "Использование: $0 [выходная_папка] [папка_с_данными] [--zip]"
        echo ""
        echo "Параметры:"
        echo "  выходная_папка    - куда собрать файлы (по умолчанию: ./switch_bully_ready)"
        echo "  папка_с_данными   - где находятся bullyorig/ и bully/ (по умолчанию: ./bully_data)"
        echo "  --zip             - создать ZIP архив после сборки"
        echo ""
        echo "Примеры:"
        echo "  $0                                    # Базовое использование"
        echo "  $0 ./ready                            # Указать выходную папку"
        echo "  $0 ./ready ./bully_data               # Указать обе папки"
        echo "  $0 ./ready ./bully_data --zip         # С созданием архива"
        exit 0
    elif [ -z "$OUTPUT_DIR_SET" ]; then
        OUTPUT_DIR="$arg"
        OUTPUT_DIR_SET=1
    elif [ -z "$DATA_DIR_SET" ]; then
        BULLY_DATA_DIR="$arg"
        DATA_DIR_SET=1
    fi
done

echo "=========================================="
echo "Сборка Bully для Nintendo Switch"
echo "=========================================="
echo "Выходная папка: $OUTPUT_DIR"
echo ""

cd "$SCRIPT_DIR"

# Шаг 1: Сборка проекта
echo "1. Сборка проекта..."
echo "----------------------------------------"
if [ ! -f "Makefile" ]; then
    echo "   ❌ Makefile не найден!"
    exit 1
fi

make clean > /dev/null 2>&1 || true
make

if [ ! -f "project.nro" ]; then
    echo "   ❌ Ошибка: project.nro не создан!"
    exit 1
fi

echo "   ✅ Проект собран: project.nro"
echo ""

# Шаг 2: Проверка libGame.so
echo "2. Проверка libGame.so..."
echo "----------------------------------------"
if [ ! -f "libGame.so" ]; then
    echo "   ⚠️  libGame.so не найден в текущей папке"
    echo "   Ищу в других местах..."
    
    # Проверяем в extracted_libs
    if [ -f "/home/ev/Downloads/bully_archive/Bully_v1.4.277.build.3793806/extracted_libs/libGame.so" ]; then
        echo "   ✅ Найден в extracted_libs, копирую..."
        cp "/home/ev/Downloads/bully_archive/Bully_v1.4.277.build.3793806/extracted_libs/libGame.so" .
    else
        echo "   ❌ libGame.so не найден!"
        echo "   Пожалуйста, скопируйте libGame.so в папку проекта"
        exit 1
    fi
else
    echo "   ✅ libGame.so найден"
fi
echo ""

# Шаг 3: Извлечение данных игры
echo "3. Извлечение данных игры..."
echo "----------------------------------------"
if [ ! -d "$BULLY_DATA_DIR/bullyorig" ] || [ ! -d "$BULLY_DATA_DIR/bully" ]; then
    echo "   ⚠️  Данные игры не найдены в $BULLY_DATA_DIR"
    echo "   Запускаю извлечение данных..."
    
    if [ ! -f "extract_bully_data.sh" ]; then
        echo "   ❌ extract_bully_data.sh не найден!"
        exit 1
    fi
    
    chmod +x extract_bully_data.sh
    ./extract_bully_data.sh "$BULLY_DATA_DIR"
    
    if [ ! -d "$BULLY_DATA_DIR/bullyorig" ] || [ ! -d "$BULLY_DATA_DIR/bully" ]; then
        echo "   ❌ Ошибка при извлечении данных!"
        exit 1
    fi
else
    echo "   ✅ Данные игры уже извлечены"
fi
echo ""

# Шаг 4: Создание выходной папки
echo "4. Создание выходной папки..."
echo "----------------------------------------"
rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

echo "   ✅ Папка создана: $OUTPUT_DIR"
echo ""

# Шаг 5: Копирование файлов
echo "5. Копирование файлов..."
echo "----------------------------------------"

# Основные файлы
echo "   Копирую project.nro..."
cp project.nro "$OUTPUT_DIR/"

echo "   Копирую libGame.so..."
cp libGame.so "$OUTPUT_DIR/"

# Опционально: config.txt (если есть)
if [ -f "config.txt" ]; then
    echo "   Копирую config.txt..."
    cp config.txt "$OUTPUT_DIR/"
else
    echo "   ℹ️  config.txt не найден (создастся автоматически при запуске)"
fi

# Данные игры
echo "   Копирую bullyorig/..."
cp -r "$BULLY_DATA_DIR/bullyorig" "$OUTPUT_DIR/"

echo "   Копирую bully/..."
cp -r "$BULLY_DATA_DIR/bully" "$OUTPUT_DIR/"

echo "   ✅ Все файлы скопированы"
echo ""

# Шаг 6: Проверка структуры
echo "6. Проверка структуры..."
echo "----------------------------------------"
cd "$OUTPUT_DIR"

REQUIRED_FILES=(
    "project.nro"
    "libGame.so"
    "bullyorig/version.cfg"
    "bullyorig/runtime.xml"
    "bullyorig/memory.xml"
    "bullyorig/config"
    "bullyorig/models"
    "bullyorig/audio"
    "bullyorig/data"
)

MISSING_FILES=0
for file in "${REQUIRED_FILES[@]}"; do
    if [ ! -e "$file" ]; then
        echo "   ❌ Отсутствует: $file"
        MISSING_FILES=$((MISSING_FILES + 1))
    fi
done

if [ $MISSING_FILES -eq 0 ]; then
    echo "   ✅ Все обязательные файлы на месте"
else
    echo "   ⚠️  Отсутствует $MISSING_FILES файл(ов)"
fi
echo ""

# Шаг 7: Подсчет размеров
echo "7. Информация о размерах..."
echo "----------------------------------------"
TOTAL_SIZE=$(du -sh . | cut -f1)
echo "   Общий размер: $TOTAL_SIZE"
echo ""
echo "   Размеры компонентов:"
du -sh project.nro libGame.so bullyorig/ bully/ 2>/dev/null | sed 's|^|   |'
echo ""

# Шаг 8: Создание README
echo "8. Создание README.txt..."
echo "----------------------------------------"
cat > README.txt << 'EOF'
========================================
Bully для Nintendo Switch
========================================

ИНСТРУКЦИЯ ПО УСТАНОВКЕ:

1. Скопируйте ВСЮ папку "bully" на SD карту Switch:
   SD:/switch/bully/

2. Структура должна быть:
   /switch/bully/
   ├── project.nro
   ├── libGame.so
   ├── config.txt (создается автоматически)
   ├── bullyorig/
   └── bully/

3. Запустите через Homebrew Menu на Switch

ВАЖНО:
- Убедитесь, что на SD карте достаточно места (~1GB)
- Все файлы должны быть в одной папке /switch/bully/
- config.txt создастся автоматически при первом запуске
- Сохранения будут в папке savegames/

РАЗМЕРЫ:
- project.nro: ~несколько MB
- libGame.so: ~19MB
- bullyorig/: ~168MB
- bully/: ~825MB
- ИТОГО: ~1GB

ПРОБЛЕМЫ?
- Проверьте логи в debug.log
- Убедитесь, что все файлы скопированы
- Проверьте права доступа на файлы

========================================
EOF
echo "   ✅ README.txt создан"
echo ""

# Шаг 9: Опционально - создание архива
if [ "$CREATE_ZIP" = true ]; then
    echo "9. Создание ZIP архива..."
    echo "----------------------------------------"
    ARCHIVE_NAME="bully_switch_$(date +%Y%m%d_%H%M%S).zip"
    echo "   Создаю архив: $ARCHIVE_NAME"
    cd "$SCRIPT_DIR"
    zip -r "$ARCHIVE_NAME" "$(basename $OUTPUT_DIR)" > /dev/null 2>&1
    ARCHIVE_SIZE=$(du -sh "$ARCHIVE_NAME" | cut -f1)
    echo "   ✅ Архив создан: $ARCHIVE_NAME ($ARCHIVE_SIZE)"
    echo "   Расположение: $SCRIPT_DIR/$ARCHIVE_NAME"
    echo ""
else
    echo "9. Создание архива..."
    echo "----------------------------------------"
    echo "   ℹ️  Пропущено (используйте --zip для создания архива)"
    echo ""
fi

# Итоги
echo "=========================================="
echo "✅ ГОТОВО!"
echo "=========================================="
echo ""
echo "Папка для Switch готова:"
echo "  $OUTPUT_DIR"
echo ""
echo "Следующие шаги:"
echo "  1. Скопируйте ВСЁ содержимое папки '$OUTPUT_DIR' на SD карту"
echo "  2. Путь должен быть: SD:/switch/bully/"
echo "  3. Запустите через Homebrew Menu на Switch"
echo ""
if [ "$CREATE_ZIP" = true ] && [ -f "$SCRIPT_DIR/$ARCHIVE_NAME" ]; then
    echo "Или используйте архив:"
    echo "  $SCRIPT_DIR/$ARCHIVE_NAME"
    echo "  (распакуйте его на SD карту в /switch/bully/)"
    echo ""
fi
echo "Размер: $TOTAL_SIZE"
echo ""
echo "Использование:"
echo "  ./build_for_switch.sh [папка] [данные] [--zip]"
echo "  Пример: ./build_for_switch.sh ./ready ./bully_data --zip"
echo ""
