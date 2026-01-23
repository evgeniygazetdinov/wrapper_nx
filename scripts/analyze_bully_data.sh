#!/bin/bash
# Скрипт для анализа структуры файлов данных Bully

DATA_DIR="/home/ev/Downloads/bully_archive/Bully_v1.4.277.build.3793806"
OUTPUT_FILE="bully_data_structure.txt"

echo "=========================================="
echo "Анализ структуры файлов данных Bully"
echo "=========================================="
echo ""

# Проверяем split_data_1.apk
if [ -f "$DATA_DIR/split_data_1.apk" ]; then
    echo "1. Анализ split_data_1.apk:"
    echo "----------------------------------------"
    
    TEMP_DIR=$(mktemp -d)
    unzip -q "$DATA_DIR/split_data_1.apk" -d "$TEMP_DIR" 2>/dev/null
    
    if [ $? -eq 0 ]; then
        echo "   ✅ Успешно распакован"
        echo ""
        echo "   Структура папок:"
        find "$TEMP_DIR" -type d | sed 's|^|   |' | head -30
        echo ""
        echo "   Файлы в корне:"
        ls -lh "$TEMP_DIR" | grep -v "^d" | sed 's|^|   |' | head -20
        echo ""
        echo "   Все файлы (первые 50):"
        find "$TEMP_DIR" -type f | sed 's|^|   |' | head -50
        echo ""
        echo "   Расширения файлов:"
        find "$TEMP_DIR" -type f -name "*.*" | sed 's/.*\.//' | sort | uniq -c | sort -rn | sed 's|^|   |' | head -20
    else
        echo "   ❌ Не удалось распаковать"
    fi
    
    rm -rf "$TEMP_DIR"
else
    echo "   ⚠️  split_data_1.apk не найден"
fi

echo ""

# Проверяем OBB файлы
echo "2. Поиск OBB файлов:"
echo "----------------------------------------"
OBB_FILES=$(find "$DATA_DIR" -name "*.obb" -type f 2>/dev/null)

if [ ! -z "$OBB_FILES" ]; then
    for OBB in $OBB_FILES; do
        echo "   Найден: $(basename $OBB)"
        echo "   Размер: $(du -h "$OBB" | cut -f1)"
        echo ""
        echo "   Содержимое (первые 50 файлов):"
        unzip -l "$OBB" 2>/dev/null | tail -n +4 | head -50 | sed 's|^|   |'
        echo ""
    done
else
    echo "   ⚠️  OBB файлы не найдены"
fi

echo ""

# Проверяем base.apk на наличие assets
echo "3. Проверка assets в base.apk:"
echo "----------------------------------------"
if [ -f "$DATA_DIR/base.apk" ]; then
    TEMP_DIR=$(mktemp -d)
    unzip -q "$DATA_DIR/base.apk" -d "$TEMP_DIR" 2>/dev/null
    
    if [ -d "$TEMP_DIR/assets" ]; then
        echo "   ✅ Папка assets найдена"
        echo "   Структура:"
        find "$TEMP_DIR/assets" -type d | sed 's|^|   |' | head -20
        echo ""
        echo "   Файлы (первые 30):"
        find "$TEMP_DIR/assets" -type f | sed 's|^|   |' | head -30
    else
        echo "   ❌ Папка assets не найдена"
    fi
    
    rm -rf "$TEMP_DIR"
else
    echo "   ⚠️  base.apk не найден"
fi

echo ""
echo "=========================================="
echo "Анализ завершен"
echo "=========================================="
