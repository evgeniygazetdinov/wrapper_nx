#!/bin/bash
# Скрипт для поиска arm64-v8a библиотек в APK, OBB и других местах

if [ $# -eq 0 ]; then
    echo "Использование: $0 <путь_к_apk_файлу> [путь_к_obb_файлу]"
    echo "Пример: $0 bully.apk bully.obb"
    exit 1
fi

APK_FILE="$1"
OBB_FILE="$2"

echo "=========================================="
echo "Поиск arm64-v8a библиотек для Bully"
echo "=========================================="
echo ""

FOUND_ARM64=false

# Функция для проверки APK
check_apk() {
    local apk="$1"
    local name="$2"
    
    if [ ! -f "$apk" ]; then
        return
    fi
    
    echo "Проверка: $name"
    echo "----------------------------------------"
    
    TEMP_DIR=$(mktemp -d)
    unzip -q "$apk" -d "$TEMP_DIR" 2>/dev/null
    
    if [ $? -ne 0 ]; then
        rm -rf "$TEMP_DIR"
        return
    fi
    
    # Ищем arm64-v8a в основном APK
    if [ -d "$TEMP_DIR/lib/arm64-v8a" ]; then
        echo "   ✅ Найдена папка lib/arm64-v8a/"
        find "$TEMP_DIR/lib/arm64-v8a" -name "*.so" -type f | while read so_file; do
            echo "      📦 $(basename $so_file)"
        done
        FOUND_ARM64=true
    fi
    
    # Ищем вложенные APK в assets
    NESTED_APKS=$(find "$TEMP_DIR" -path "*/assets/*.apk" -type f)
    for NESTED_APK in $NESTED_APKS; do
        NESTED_NAME=$(basename "$NESTED_APK")
        NESTED_TEMP=$(mktemp -d)
        unzip -q "$NESTED_APK" -d "$NESTED_TEMP" 2>/dev/null
        
        if [ -d "$NESTED_TEMP/lib/arm64-v8a" ]; then
            echo "   ✅ В $NESTED_NAME найдена папка lib/arm64-v8a/"
            find "$NESTED_TEMP/lib/arm64-v8a" -name "*.so" -type f | while read so_file; do
                echo "      📦 $(basename $so_file) (в $NESTED_NAME)"
            done
            FOUND_ARM64=true
        fi
        
        rm -rf "$NESTED_TEMP"
    done
    
    rm -rf "$TEMP_DIR"
    echo ""
}

# Функция для проверки OBB
check_obb() {
    local obb="$1"
    
    if [ ! -f "$obb" ]; then
        return
    fi
    
    echo "Проверка OBB файла: $(basename $obb)"
    echo "----------------------------------------"
    
    TEMP_DIR=$(mktemp -d)
    unzip -q "$obb" -d "$TEMP_DIR" 2>/dev/null
    
    if [ $? -ne 0 ]; then
        echo "   ⚠️  Не удалось распаковать OBB (возможно, это не ZIP)"
        rm -rf "$TEMP_DIR"
        echo ""
        return
    fi
    
    # Ищем arm64-v8a в OBB
    if [ -d "$TEMP_DIR/lib/arm64-v8a" ]; then
        echo "   ✅ Найдена папка lib/arm64-v8a/ в OBB!"
        find "$TEMP_DIR/lib/arm64-v8a" -name "*.so" -type f | while read so_file; do
            echo "      📦 $(basename $so_file)"
        done
        FOUND_ARM64=true
    else
        # Ищем любые .so файлы в OBB
        SO_FILES=$(find "$TEMP_DIR" -name "*.so" -type f)
        if [ ! -z "$SO_FILES" ]; then
            echo "   ⚠️  Найдены .so файлы, но не в lib/arm64-v8a/:"
            echo "$SO_FILES" | sed 's|^|      |'
            echo "   Проверьте архитектуру вручную:"
            echo "$SO_FILES" | head -1 | xargs file 2>/dev/null | sed 's|^|      |'
        fi
    fi
    
    rm -rf "$TEMP_DIR"
    echo ""
}

# Проверяем основной APK
if [ -f "$APK_FILE" ]; then
    check_apk "$APK_FILE" "Основной APK"
fi

# Проверяем OBB
if [ ! -z "$OBB_FILE" ] && [ -f "$OBB_FILE" ]; then
    check_obb "$OBB_FILE"
fi

# Итоги
echo "=========================================="
echo "Результат поиска"
echo "=========================================="
if [ "$FOUND_ARM64" = true ]; then
    echo "✅ arm64-v8a библиотеки НАЙДЕНЫ!"
    echo ""
    echo "Следующие шаги:"
    echo "1. Извлеките .so файл из найденного места"
    echo "2. Используйте скрипт: ./extract_so_from_nested_apk.sh $APK_FILE arm64-v8a"
    echo "3. Или извлеките вручную из OBB, если там найдены библиотеки"
else
    echo "❌ arm64-v8a библиотеки НЕ НАЙДЕНЫ"
    echo ""
    echo "Возможные решения:"
    echo "1. Проверьте другую версию APK Bully"
    echo "2. Убедитесь, что у вас есть OBB файл и проверьте его"
    echo "3. Проверьте split APK файлы (если есть)"
    echo "4. Возможно, эта версия Bully не содержит 64-bit библиотек"
    echo ""
    echo "⚠️  ВАЖНО: armeabi-v7a (32-bit) НЕ ПОДОЙДЕТ!"
    echo "   Wrapper требует arm64-v8a (64-bit) архитектуру"
fi
echo ""
