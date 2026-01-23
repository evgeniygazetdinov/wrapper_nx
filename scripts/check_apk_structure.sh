#!/bin/bash
# Скрипт для проверки структуры APK и поиска .so файлов

if [ $# -eq 0 ]; then
    echo "Использование: $0 <путь_к_apk_файлу>"
    echo "Пример: $0 bully.apk"
    exit 1
fi

APK_FILE="$1"

if [ ! -f "$APK_FILE" ]; then
    echo "Ошибка: Файл $APK_FILE не найден!"
    exit 1
fi

echo "=========================================="
echo "Анализ APK: $APK_FILE"
echo "=========================================="
echo ""

# Создаем временную папку
TEMP_DIR=$(mktemp -d)
echo "Распаковка в: $TEMP_DIR"
echo ""

# Распаковываем APK
unzip -q "$APK_FILE" -d "$TEMP_DIR" 2>/dev/null

if [ $? -ne 0 ]; then
    echo "Ошибка: Не удалось распаковать APK. Возможно, это не ZIP архив."
    rm -rf "$TEMP_DIR"
    exit 1
fi

echo "1. Поиск .so файлов в APK:"
echo "----------------------------------------"
SO_FILES=$(find "$TEMP_DIR" -name "*.so" -type f)
if [ -z "$SO_FILES" ]; then
    echo "   ❌ .so файлы НЕ НАЙДЕНЫ в APK!"
else
    echo "   ✅ Найдены .so файлы:"
    echo "$SO_FILES" | sed 's|^|   |'
fi
echo ""

echo "2. Структура папок в APK:"
echo "----------------------------------------"
ls -la "$TEMP_DIR" | grep "^d" | awk '{print "   " $9}' | grep -v "^\.$"
echo ""

echo "3. Проверка папки lib/:"
echo "----------------------------------------"
if [ -d "$TEMP_DIR/lib" ]; then
    echo "   ✅ Папка lib/ найдена!"
    echo "   Содержимое:"
    find "$TEMP_DIR/lib" -type f -o -type d | sed 's|^|   |'
else
    echo "   ❌ Папка lib/ НЕ найдена в APK"
fi
echo ""

echo "4. Проверка папки assets/:"
echo "----------------------------------------"
if [ -d "$TEMP_DIR/assets" ]; then
    echo "   ✅ Папка assets/ найдена!"
    echo "   Первые файлы/папки:"
    ls -la "$TEMP_DIR/assets" | head -10 | sed 's|^|   |'
    
    # Ищем .so в assets
    SO_IN_ASSETS=$(find "$TEMP_DIR/assets" -name "*.so" -type f)
    if [ ! -z "$SO_IN_ASSETS" ]; then
        echo "   ⚠️  Найдены .so файлы в assets/:"
        echo "$SO_IN_ASSETS" | sed 's|^|   |'
    fi
else
    echo "   ❌ Папка assets/ НЕ найдена"
fi
echo ""

echo "5. Проверка AndroidManifest.xml:"
echo "----------------------------------------"
if [ -f "$TEMP_DIR/AndroidManifest.xml" ]; then
    echo "   ✅ AndroidManifest.xml найден"
    # Пытаемся найти информацию о пакете (если есть aapt)
    if command -v aapt &> /dev/null; then
        echo "   Информация о пакете:"
        aapt dump badging "$APK_FILE" 2>/dev/null | grep -E "package|native-code" | sed 's|^|   |'
    fi
else
    echo "   ❌ AndroidManifest.xml не найден"
fi
echo ""

echo "6. Все файлы с расширением .so, .dex, .apk:"
echo "----------------------------------------"
find "$TEMP_DIR" -type f \( -name "*.so" -o -name "*.dex" -o -name "*.apk" \) | sed 's|^|   |'
echo ""

echo "7. Проверка вложенных APK файлов (в assets/):"
echo "----------------------------------------"
NESTED_APKS=$(find "$TEMP_DIR" -path "*/assets/*.apk" -type f)
if [ ! -z "$NESTED_APKS" ]; then
    echo "   ✅ Найдены вложенные APK файлы:"
    echo "$NESTED_APKS" | sed 's|^|   |'
    echo ""
    echo "   Распаковка вложенных APK для поиска .so файлов..."
    for NESTED_APK in $NESTED_APKS; do
        NESTED_NAME=$(basename "$NESTED_APK")
        NESTED_TEMP=$(mktemp -d)
        echo "   Проверка: $NESTED_NAME"
        unzip -q "$NESTED_APK" -d "$NESTED_TEMP" 2>/dev/null
        if [ $? -eq 0 ]; then
            NESTED_SO=$(find "$NESTED_TEMP" -name "*.so" -type f)
            if [ ! -z "$NESTED_SO" ]; then
                echo "   ✅ В $NESTED_NAME найдены .so файлы:"
                echo "$NESTED_SO" | sed "s|$NESTED_TEMP|      |" | sed 's|^|   |'
                # Проверяем архитектуру
                for SO_FILE in $NESTED_SO; do
                    if echo "$SO_FILE" | grep -q "arm64\|aarch64"; then
                        echo "      ⭐ ARM64 файл: $(basename $SO_FILE)"
                    fi
                done
            else
                echo "   ❌ В $NESTED_NAME .so файлы не найдены"
            fi
            # Проверяем структуру вложенного APK
            if [ -d "$NESTED_TEMP/lib" ]; then
                echo "   📁 Структура lib/ в $NESTED_NAME:"
                find "$NESTED_TEMP/lib" -type d | sed "s|$NESTED_TEMP|      |" | sed 's|^|   |'
            fi
        fi
        rm -rf "$NESTED_TEMP"
    done
else
    echo "   ❌ Вложенные APK файлы не найдены"
fi
echo ""

echo "8. Рекомендации:"
echo "----------------------------------------"
if [ -z "$SO_FILES" ] && [ -z "$NESTED_APKS" ]; then
    echo "   ⚠️  .so файлы не найдены в APK!"
    echo "   Попробуйте:"
    echo "   1. Проверить OBB файл (распакуйте его как ZIP)"
    echo "   2. Проверить split APK файлы (если есть)"
    echo "   3. Убедиться, что это правильная версия APK"
    echo "   4. Попробовать другую версию APK"
elif [ ! -z "$NESTED_APKS" ]; then
    echo "   ✅ Найден вложенный APK в assets/!"
    echo "   Следующие шаги:"
    echo "   1. Распакуйте install.apk (или другой найденный APK) из assets/"
    echo "   2. Найдите .so файл в lib/arm64-v8a/ внутри вложенного APK"
    echo "   3. Скопируйте .so файл в папку проекта wrapper"
    echo "   4. Используйте его имя в source/config.h (SO_NAME)"
    echo "   5. Проанализируйте символы в .so файле"
    echo ""
    echo "   Команда для извлечения:"
    echo "   unzip -j bully.apk 'assets/install.apk' -d /tmp/"
    echo "   unzip install.apk 'lib/arm64-v8a/*.so' -d /tmp/bully_libs/"
elif [ ! -z "$SO_FILES" ]; then
    echo "   ✅ .so файлы найдены!"
    echo "   Следующие шаги:"
    echo "   1. Скопируйте нужный .so файл (arm64-v8a) в папку проекта"
    echo "   2. Используйте его имя в source/config.h (SO_NAME)"
    echo "   3. Проанализируйте символы в .so файле"
fi
echo ""

# Очистка
echo "Очистка временных файлов..."
rm -rf "$TEMP_DIR"
echo "Готово!"
