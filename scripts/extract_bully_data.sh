#!/bin/bash
# Скрипт для извлечения данных Bully из split_data_1.apk

SOURCE_DIR="/home/ev/Downloads/bully_archive/Bully_v1.4.277.build.3793806"
TARGET_DIR="${1:-./bully_data}"

if [ ! -f "$SOURCE_DIR/split_data_1.apk" ]; then
    echo "Ошибка: split_data_1.apk не найден в $SOURCE_DIR"
    echo "Использование: $0 [целевая_папка]"
    exit 1
fi

echo "=========================================="
echo "Извлечение данных Bully"
echo "=========================================="
echo "Источник: $SOURCE_DIR/split_data_1.apk"
echo "Целевая папка: $TARGET_DIR"
echo ""

# Создаем временную папку для распаковки APK
TEMP_DIR=$(mktemp -d)
echo "1. Распаковка split_data_1.apk..."
unzip -q "$SOURCE_DIR/split_data_1.apk" -d "$TEMP_DIR" 2>/dev/null

if [ $? -ne 0 ]; then
    echo "   ❌ Ошибка при распаковке split_data_1.apk"
    rm -rf "$TEMP_DIR"
    exit 1
fi

echo "   ✅ Успешно распакован"
echo ""

# Создаем целевую папку
mkdir -p "$TARGET_DIR"
cd "$TARGET_DIR"

# Распаковываем data_0.zip
echo "2. Распаковка data_0.zip (создаст папку bullyorig/)..."
if [ -f "$TEMP_DIR/assets/data_0.zip" ]; then
    unzip -q "$TEMP_DIR/assets/data_0.zip" -d . 2>/dev/null
    if [ $? -eq 0 ]; then
        echo "   ✅ data_0.zip распакован"
        if [ -d "bullyorig" ]; then
            echo "   ✅ Папка bullyorig/ создана"
            echo "   Размер: $(du -sh bullyorig | cut -f1)"
        fi
    else
        echo "   ❌ Ошибка при распаковке data_0.zip"
    fi
else
    echo "   ❌ data_0.zip не найден"
fi
echo ""

# Распаковываем data_1.zip
echo "3. Распаковка data_1.zip (создаст папку bully/)..."
if [ -f "$TEMP_DIR/assets/data_1.zip" ]; then
    unzip -q "$TEMP_DIR/assets/data_1.zip" -d . 2>/dev/null
    if [ $? -eq 0 ]; then
        echo "   ✅ data_1.zip распакован"
        if [ -d "bully" ]; then
            echo "   ✅ Папка bully/ создана"
            echo "   Размер: $(du -sh bully | cut -f1)"
        fi
    else
        echo "   ❌ Ошибка при распаковке data_1.zip"
    fi
else
    echo "   ❌ data_1.zip не найден"
fi
echo ""

# Очистка
rm -rf "$TEMP_DIR"

echo "=========================================="
echo "Готово!"
echo "=========================================="
echo "Данные извлечены в: $TARGET_DIR"
echo ""
echo "Структура:"
echo "  $TARGET_DIR/"
echo "  ├── bullyorig/"
echo "  │   ├── version.cfg"
echo "  │   ├── runtime.xml"
echo "  │   ├── config/"
echo "  │   ├── models/"
echo "  │   └── ..."
echo "  └── bully/"
echo "      ├── speech_*.snd"
echo "      └── ..."
echo ""
echo "Скопируйте содержимое $TARGET_DIR в папку игры на Switch"
