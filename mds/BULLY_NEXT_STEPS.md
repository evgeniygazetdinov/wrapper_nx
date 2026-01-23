# Следующие шаги для адаптации Bully

## ✅ Что уже сделано

1. ✅ Найдена правильная версия APK (v1.4.277)
2. ✅ Найдена arm64-v8a библиотека (`libGame.so`)
3. ✅ Библиотека извлечена и проверена
4. ✅ Найдены основные символы

## 📋 Что нужно сделать дальше

### Шаг 1: Скопировать библиотеку в проект

```bash
# Скопируйте libGame.so в папку проекта wrapper
cp /home/ev/Downloads/bully_archive/Bully_v1.4.277.build.3793806/extracted_libs/libGame.so /home/ev/my_code/wrapper_nx/
```

### Шаг 2: Изменить конфигурацию

В файле `source/config.h` измените:
```c
#define SO_NAME "libGame.so"
```

### Шаг 3: Найти точку входа в игру

Это самая сложная часть. Нужно найти функцию, которая запускает игровой цикл.

**Попробуйте найти:**
```bash
cd /home/ev/Downloads/bully_archive/Bully_v1.4.277.build.3793806/extracted_libs

# Поиск функций с "main" или "start"
nm -D libGame.so | grep -E " T " | grep -iE "main|start|entry|begin" | head -20

# Поиск всех функций, которые могут быть точкой входа
readelf -Ws libGame.so | grep "FUNC.*GLOBAL" | grep -v " UND " | grep -iE "init|main|start|entry|run|loop" | head -30
```

### Шаг 4: Адаптировать main.c

В `source/main.c` нужно будет изменить:

1. **Список файлов данных** в `check_data()`:
   - Проверьте, какие файлы есть в `split_data_1.apk`
   - Распакуйте его и посмотрите структуру

2. **Точку входа:**
   - Вместо `initGraphics()`, `ShowJoystick()`, `NVEventAppMain()`
   - Нужно найти аналогичные функции в Bully

3. **Глобальные переменные:**
   - Найти аналоги `StorageRootBuffer`, `IsAndroidPaused`, `UseRGBA8`

### Шаг 5: Адаптировать хуки

В `source/hooks/game.c`:

1. Удалить специфичные для Max Payne функции
2. Найти аналоги функций управления, экрана, файловой системы
3. Возможно, нужно будет работать с JNI интерфейсом

## 🔍 Полезные команды для анализа

```bash
cd /home/ev/Downloads/bully_archive/Bully_v1.4.277.build.3793806/extracted_libs

# Сохранить все экспортируемые функции
readelf -Ws libGame.so | grep "FUNC.*GLOBAL.*DEFAULT.*14" > /tmp/bully_functions.txt

# Поиск функций управления
nm -D libGame.so | grep -iE "input|control|button|pad|joystick|gamepad" | grep " T "

# Поиск функций экрана
nm -D libGame.so | grep -iE "screen|display|window|viewport|resolution" | grep " T "

# Поиск функций файловой системы
nm -D libGame.so | grep -iE "file|open|read|write|storage|path" | grep " T "

# Поиск глобальных переменных с путями
nm -D libGame.so | grep -E " B | D " | grep -iE "root|path|storage|data"
```

## 📦 Проверка файлов данных

```bash
# Распакуйте split_data_1.apk
cd /home/ev/Downloads/bully_archive/Bully_v1.4.277.build.3793806
unzip -l split_data_1.apk | head -30

# Или распакуйте полностью
mkdir -p data_extracted
unzip split_data_1.apk -d data_extracted/
ls -la data_extracted/
```

## ⚠️ Важные замечания

1. **Bully сложнее Max Payne:**
   - Использует JNI интерфейс
   - Использует Flutter для UI
   - Более сложная архитектура

2. **Может потребоваться больше работы:**
   - Нужно найти точку входа
   - Адаптировать больше функций
   - Возможно, нужны дополнительные библиотеки

3. **Тестирование:**
   - Начните с простых изменений
   - Тестируйте на каждом этапе
   - Читайте ошибки - они подскажут, что нужно исправить

## 📝 Документация

- `BULLY_SYMBOLS_FOUND.md` - найденные символы
- `BULLY_ADAPTATION_GUIDE.md` - общее руководство
- `check_apk_structure.sh` - скрипт проверки APK
- `find_arm64_libs.sh` - скрипт поиска библиотек

## 🎯 Приоритеты

1. **Высокий приоритет:**
   - Найти точку входа в игру
   - Определить структуру файлов данных
   - Адаптировать базовые хуки (экран, управление)

2. **Средний приоритет:**
   - Адаптировать файловую систему
   - Адаптировать Android API
   - Настроить память и инициализацию

3. **Низкий приоритет:**
   - Оптимизация
   - Дополнительные функции
   - Улучшение совместимости
