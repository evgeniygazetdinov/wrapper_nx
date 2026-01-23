#!/bin/bash
# Скрипт для извлечения .so файла из вложенного APK в assets/

if [ $# -eq 0 ]; then
    echo "Использование: $0 <путь_к_apk_файлу> [архитектура]"
    echo "Пример: $0 bully.apk arm64-v8a"
    echo ""
    echo "Архитектуры: arm64-v8a (по умолчанию), armeabi-v7a, x86, x86_64"
    exit 1
fi

APK_FILE="$1"
ARCH="${2:-arm64-v8a}"  # По умолчанию arm64-v8a

if [ ! -f "$APK_FILE" ]; then
    echo "Ошибка: Файл $APK_FILE не найден!"
    exit 1
fi

echo "=========================================="
echo "Извлечение .so из вложенного APK"
echo "=========================================="
echo "APK: $APK_FILE"
echo "Архитектура: $ARCH"
echo ""

# Создаем временную папку
TEMP_DIR=$(mktemp -d)
OUTPUT_DIR="./extracted_libs"

# Распаковываем основной APK
echo "1. Распаковка основного APK..."
unzip -q "$APK_FILE" -d "$TEMP_DIR" 2>/dev/null

if [ $? -ne 0 ]; then
    echo "Ошибка: Не удалось распаковать APK"
    rm -rf "$TEMP_DIR"
    exit 1
fi

# Ищем вложенные APK в assets
echo "2. Поиск вложенных APK в assets/..."
NESTED_APKS=$(find "$TEMP_DIR" -path "*/assets/*.apk" -type f)

if [ -z "$NESTED_APKS" ]; then
    echo "❌ Вложенные APK не найдены в assets/"
    echo "Проверьте структуру APK вручную"
    rm -rf "$TEMP_DIR"
    exit 1
fi

echo "✅ Найдены вложенные APK:"
for APK in $NESTED_APKS; do
    echo "   - $(basename $APK)"
done
echo ""

# Создаем выходную папку
mkdir -p "$OUTPUT_DIR"

# Обрабатываем каждый вложенный APK
for NESTED_APK in $NESTED_APKS; do
    NESTED_NAME=$(basename "$NESTED_APK")
    echo "3. Обработка: $NESTED_NAME"
    
    # Распаковываем вложенный APK
    NESTED_TEMP=$(mktemp -d)
    unzip -q "$NESTED_APK" -d "$NESTED_TEMP" 2>/dev/null
    
    if [ $? -ne 0 ]; then
        echo "   ⚠️  Не удалось распаковать $NESTED_NAME"
        rm -rf "$NESTED_TEMP"
        continue
    fi
    
    # Ищем .so файлы нужной архитектуры
    SO_FILES=$(find "$NESTED_TEMP" -path "*/lib/$ARCH/*.so" -type f)
    
    if [ -z "$SO_FILES" ]; then
        echo "   ⚠️  .so файлы для $ARCH не найдены в $NESTED_NAME"
        echo "   Доступные архитектуры:"
        find "$NESTED_TEMP/lib" -type d -mindepth 1 -maxdepth 1 2>/dev/null | sed 's|.*/||' | sed 's|^|      - |'
    else
        echo "   ✅ Найдены .so файлы для $ARCH:"
        for SO_FILE in $SO_FILES; do
            SO_NAME=$(basename "$SO_FILE")
            echo "      - $SO_NAME"
            
            # Копируем .so файл
            cp "$SO_FILE" "$OUTPUT_DIR/$SO_NAME"
            echo "      ✅ Скопирован в: $OUTPUT_DIR/$SO_NAME"
        done
    fi
    
    rm -rf "$NESTED_TEMP"
done

echo ""
echo "=========================================="
echo "Готово!"
echo "=========================================="
echo "Извлеченные .so файлы находятся в: $OUTPUT_DIR"
echo ""
echo "Следующие шаги:"
echo "1. Скопируйте нужный .so файл в папку вашего wrapper проекта"
echo "2. Обновите source/config.h:"
echo "   #define SO_NAME \"имя_вашего_so_файла.so\""
echo "3. Проанализируйте символы:"
echo "   nm -D $OUTPUT_DIR/*.so | grep -i init"
echo "   readelf -Ws $OUTPUT_DIR/*.so | grep -i graphics"

# Очистка
rm -rf "$TEMP_DIR"
