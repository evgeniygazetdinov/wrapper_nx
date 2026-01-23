# Ключевые функции и переменные для адаптации Bully

## ✅ Найденные ключевые функции

### Главный поток и инициализация

1. **`_Z10MainThreadPv`** (адрес: `0x96eda0`)
   - Главный поток игры
   - Вероятно, это точка входа в игровой цикл
   - Принимает `void*` параметр

2. **`_Z16AND_ThreadOnMainv`** (адрес: `0x1152c54`)
   - Android поток на главном потоке
   - Может быть нужна для инициализации Android части

3. **`_Z19AND_GameStartupDonev`** (адрес: `0x114fad4`)
   - Завершение запуска игры
   - Вызывается после инициализации

### JNI функции (связь с Java)

4. **`Java_com_rockstargames_oswrapper_GameNative_implOnInitialSetup`** (адрес: `0x114fb38`)
   - Начальная настройка игры
   - Вызывается из Java кода при запуске
   - **Возможно, это точка входа!**

5. **`Java_com_rockstargames_oswrapper_GameNative_implOnDrawFrame`** (адрес: `0x114fda4`)
   - Отрисовка кадра
   - Вызывается каждый кадр из Java кода
   - Аналог игрового цикла

6. **`Java_com_rockstargames_oswrapper_GameNative_implIsInitialized`** (адрес: `0x114fb24`)
   - Проверка инициализации
   - Возвращает статус инициализации

## ✅ Найденные глобальные переменные

1. **`StorageRootPath`** (адрес: `0x15845c0`)
   - Путь к корню хранилища данных
   - **Аналог `StorageRootBuffer` из Max Payne!**

2. **`StorageBaseRootPath`** (адрес: `0x15845d8`)
   - Базовый путь хранилища
   - Дополнительный путь для данных

3. **`ANDThreadStorageKey`** (адрес: `0x1584550`)
   - Ключ хранилища Android потока

## 🔧 Рекомендации по адаптации

### Вариант 1: Использовать JNI функции (рекомендуется)

Bully использует JNI интерфейс, поэтому нужно эмулировать вызовы из Java:

```c
// В main.c, после загрузки библиотеки:

// Установить путь к данным
strcpy((char *)so_find_addr("StorageRootPath"), ".");

// Вызвать начальную настройку
void (* implOnInitialSetup)(void) = (void *)so_find_addr_rx("Java_com_rockstargames_oswrapper_GameNative_implOnInitialSetup");
if (implOnInitialSetup) {
    implOnInitialSetup();
}

// Запустить главный поток
void* (* MainThread)(void*) = (void *)so_find_addr_rx("_Z10MainThreadPv");
if (MainThread) {
    // Запустить в отдельном потоке или напрямую
    MainThread(NULL);
}
```

### Вариант 2: Использовать прямые функции

Если JNI функции требуют Java окружение, можно попробовать прямые функции:

```c
// Инициализация Android части
void (* AND_ThreadOnMain)(void) = (void *)so_find_addr_rx("_Z16AND_ThreadOnMainv");
if (AND_ThreadOnMain) {
    AND_ThreadOnMain();
}

// Запуск главного потока
void* (* MainThread)(void*) = (void *)so_find_addr_rx("_Z10MainThreadPv");
if (MainThread) {
    MainThread(NULL);
}

// Сигнал о завершении запуска
void (* AND_GameStartupDone)(void) = (void *)so_find_addr_rx("_Z19AND_GameStartupDonev");
if (AND_GameStartupDone) {
    AND_GameStartupDone();
}
```

## 📝 Изменения в main.c

### 1. Изменить глобальные переменные

Замените:
```c
strcpy((char *)so_find_addr("StorageRootBuffer"), ".");
```

На:
```c
strcpy((char *)so_find_addr("StorageRootPath"), ".");
// Или также установить StorageBaseRootPath, если нужно
```

### 2. Изменить точку входа

Вместо:
```c
uint32_t (* initGraphics)(void) = (void *)so_find_addr_rx("_Z12initGraphicsv");
uint32_t (* ShowJoystick)(int show) = (void *)so_find_addr_rx("_Z12ShowJoystickb");
int (* NVEventAppMain)(int argc, char *argv[]) = (void *)so_find_addr_rx("_Z14NVEventAppMainiPPc");

initGraphics();
ShowJoystick(0);
NVEventAppMain(0, NULL);
```

Попробуйте:
```c
// Вариант A: JNI функции
void (* implOnInitialSetup)(void) = (void *)so_find_addr_rx("Java_com_rockstargames_oswrapper_GameNative_implOnInitialSetup");
void* (* MainThread)(void*) = (void *)so_find_addr_rx("_Z10MainThreadPv");

if (implOnInitialSetup) implOnInitialSetup();
if (MainThread) MainThread(NULL);

// Вариант B: Прямые функции
void (* AND_ThreadOnMain)(void) = (void *)so_find_addr_rx("_Z16AND_ThreadOnMainv");
void* (* MainThread)(void*) = (void *)so_find_addr_rx("_Z10MainThreadPv");
void (* AND_GameStartupDone)(void) = (void *)so_find_addr_rx("_Z19AND_GameStartupDonev");

if (AND_ThreadOnMain) AND_ThreadOnMain();
if (MainThread) MainThread(NULL);
if (AND_GameStartupDone) AND_GameStartupDone();
```

## ⚠️ Важные замечания

1. **JNI функции могут требовать Java окружение:**
   - Если функции не работают, возможно, нужна эмуляция JNI
   - Или нужно найти внутренние функции, которые они вызывают

2. **MainThread может быть блокирующим:**
   - Возможно, нужно запускать в отдельном потоке
   - Или это бесконечный цикл, который нужно запускать в фоне

3. **Порядок вызовов важен:**
   - Сначала установить пути
   - Потом инициализация
   - Потом запуск главного потока

## 🔍 Дополнительные исследования

Если функции не работают, попробуйте найти:

1. **Внутренние функции, вызываемые из JNI:**
```bash
# Поиск функций, которые могут вызываться из JNI
nm -D libGame.so | grep " T " | grep -iE "game.*init|init.*game|start.*game|run.*game"
```

2. **Функции инициализации графики:**
```bash
nm -D libGame.so | grep " T " | grep -iE "graphics|render|display|egl|gl"
```

3. **Функции управления:**
```bash
nm -D libGame.so | grep " T " | grep -iE "input|control|button|pad|joystick"
```

## 📋 План тестирования

1. ✅ Изменить `SO_NAME` на `"libGame.so"`
2. ✅ Изменить `StorageRootBuffer` на `StorageRootPath`
3. ⏳ Попробовать вариант A (JNI функции)
4. ⏳ Если не работает, попробовать вариант B (прямые функции)
5. ⏳ Если не работает, искать внутренние функции
6. ⏳ Адаптировать хуки в `game.c`
7. ⏳ Определить структуру файлов данных
