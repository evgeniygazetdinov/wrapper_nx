# Найденные символы в Bully Android (libGame.so)

## ✅ Найдена правильная версия!

**Версия:** Bully v1.4.277.build.3793806  
**Архитектура:** arm64-v8a (64-bit) ✅  
**Основная библиотека:** `libGame.so` (19MB)

## Извлеченные библиотеки

Из `split_config.arm64_v8a.apk` извлечены:
- `libGame.so` (19MB) - **основная библиотека игры**
- `libapp.so` (2.7MB) - приложение/UI
- `libflutter.so` (11MB) - Flutter движок (UI)
- `libopenal.so` (763KB) - звуковая библиотека
- `libc++_shared.so` (1.3MB) - стандартная C++ библиотека
- Остальные вспомогательные библиотеки

## Найденные важные символы

### JNI функции (связь с Java кодом)

```
Java_com_rockstargames_oswrapper_GameNative_implIsInitialized
Java_com_rockstargames_oswrapper_GameNative_implOnInitialSetup
Java_com_rockstargames_oswrapper_GameNative_implOnRockstarInitialComplete
Java_com_rockstargames_oswrapper_GameNative_implOnDrawFrame
Java_com_rockstargames_oswrapper_GameNative_implOnTouchMove
Java_com_rockstargames_oswrapper_GameNative_implOnTouchEnd
Java_com_rockstargames_oswrapper_GameNative_implOnAccelerometerChanged
Java_com_rockstargames_oswrapper_GameNative_implOnNetworkChanged
```

### Глобальные переменные

```
ANDThread_Initted          - флаг инициализации Android потока
ANDThreadStorageKey        - ключ хранилища потока
gameAlreadyInitialised     - флаг инициализации игры
gMainHeap                  - главная куча памяти
f_bMemallocInitialized     - флаг инициализации памяти
```

### Функции инициализации

```
CdStreamInit               - инициализация потоков данных
_ZN17DefinitionManagerC1Ev - конструктор менеджера определений
_ZN18btConvexPolyhedron10initializeEv - инициализация физики
```

## ⚠️ Отличия от Max Payne

Bully использует **другую архитектуру**, чем Max Payne:

1. **JNI интерфейс:** Bully использует JNI функции для связи с Java кодом
2. **Flutter UI:** Игра использует Flutter для интерфейса
3. **Другая структура:** Нет простых функций типа `initGraphics()` и `NVEventAppMain()`

## Что нужно сделать для адаптации

### 1. Изменить имя библиотеки

В `source/config.h`:
```c
#define SO_NAME "libGame.so"
```

### 2. Найти точку входа

Нужно найти функцию, которая запускает игру. Возможные варианты:
- JNI функции могут вызывать внутренние функции
- Нужно найти аналог `NVEventAppMain` или точку входа в игровой цикл

**Команды для поиска:**
```bash
# Поиск функций с "main" или "start"
nm -D libGame.so | grep -i "main\|start" | grep " T "

# Поиск всех экспортируемых функций
readelf -Ws libGame.so | grep "FUNC.*GLOBAL.*DEFAULT" | grep -v " UND "

# Поиск в строках
strings libGame.so | grep -iE "native.*init|game.*start|main.*loop"
```

### 3. Адаптировать хуки

В `source/hooks/game.c` нужно будет:

1. **Удалить специфичные для Max Payne хуки:**
   - `MaxPayne_ConfiguredInput_readCrouch`
   - `WeaponSwiper::Draw`
   - Другие специфичные функции

2. **Найти аналоги функций в Bully:**
   - Функции управления (аналог `WarGamepad_*`)
   - Функции экрана (аналог `OS_ScreenGet*`)
   - Функции файловой системы
   - Функции Android API

3. **Адаптировать JNI хуки:**
   - Bully использует JNI, нужно будет перехватывать JNI вызовы
   - Или найти внутренние функции, которые вызываются из JNI

### 4. Проверить глобальные переменные

Нужно найти аналоги:
- `StorageRootBuffer` - путь к данным
- `IsAndroidPaused` - флаг паузы
- `UseRGBA8` - формат текстуры

**Команды:**
```bash
# Поиск глобальных переменных
nm -D libGame.so | grep " B \| D " | grep -iE "storage|root|path|paused|rgba|format"

# Поиск в строках
strings libGame.so | grep -iE "storage|root|path|paused"
```

### 5. Проверить файлы данных

В `source/main.c`, функция `check_data()`:
- Нужно определить, какие файлы нужны Bully
- Проверить структуру OBB файла
- Возможно, нужны файлы из `split_data_1.apk`

## Следующие шаги

1. ✅ Библиотека найдена (`libGame.so`)
2. ⏳ Найти точку входа в игру
3. ⏳ Найти аналоги функций из Max Payne
4. ⏳ Адаптировать хуки
5. ⏳ Определить структуру файлов данных
6. ⏳ Протестировать на Switch

## Полезные команды для анализа

```bash
# Полный список экспортируемых функций
readelf -Ws libGame.so | grep "FUNC.*GLOBAL.*DEFAULT.*14" > bully_exports.txt

# Поиск всех глобальных переменных
nm -D libGame.so | grep " B \| D " > bully_globals.txt

# Поиск строк с путями и настройками
strings libGame.so | grep -E "\.(obb|apk|so|dat|pak)" > bully_paths.txt

# Проверка зависимостей
readelf -d libGame.so | grep NEEDED
```

## Примечания

- Bully использует более сложную архитектуру, чем Max Payne
- Возможно, потребуется больше работы по адаптации
- JNI интерфейс может усложнить портирование
- Flutter UI может требовать дополнительных библиотек
