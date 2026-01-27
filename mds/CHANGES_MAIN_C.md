# Изменения в main.c для Bully

## ✅ Внесенные изменения

### 1. Функция `check_data()` (строки 58-85)

**Было:** Проверка файлов Max Payne
```c
"MaxPayneSoundsv2.msf",
"x_data.ras",
"x_english.ras",
...
```

**Стало:** Минимальная проверка (только `data`)
```c
"data",
// TODO: Add Bully-specific data files here
```

**Примечание:** После проверки структуры OBB или `split_data_1.apk` нужно будет добавить правильные файлы Bully.

### 2. Глобальные переменные (строки 148-150)

**Было:**
```c
strcpy((char *)so_find_addr("StorageRootBuffer"), ".");
*(uint8_t *)so_find_addr("IsAndroidPaused") = 0;
*(uint8_t *)so_find_addr("UseRGBA8") = 1;
```

**Стало:**
```c
strcpy((char *)so_find_addr("StorageRootPath"), ".");
// StorageBaseRootPath можно установить при необходимости
```

**Изменения:**
- `StorageRootBuffer` → `StorageRootPath` (имя переменной в Bully)
- Убраны `IsAndroidPaused` и `UseRGBA8` (не найдены в Bully, возможно не нужны)

### 3. Точка входа в игру (строки 152-188)

**Было (Max Payne):**
```c
uint32_t (* initGraphics)(void) = ...;
uint32_t (* ShowJoystick)(int show) = ...;
int (* NVEventAppMain)(int argc, char *argv[]) = ...;

initGraphics();
ShowJoystick(0);
NVEventAppMain(0, NULL);
```

**Стало (Bully):**
```c
void* (* MainThread)(void*) = ...;
void (* AND_ThreadOnMain)(void) = ...;
void (* AND_GameStartupDone)(void) = ...;
void (* implOnInitialSetup)(void) = ...;

// Инициализация Android потока
if (AND_ThreadOnMain) AND_ThreadOnMain();

// JNI начальная настройка (если доступна)
if (implOnInitialSetup) implOnInitialSetup();

// Запуск главного потока игры
if (MainThread) MainThread(NULL);

// Сигнал о завершении запуска
if (AND_GameStartupDone) AND_GameStartupDone();
```

## 🔍 Найденные функции

1. **`_Z10MainThreadPv`** - Главный поток игры (точка входа)
2. **`_Z16AND_ThreadOnMainv`** - Инициализация Android потока
3. **`_Z19AND_GameStartupDonev`** - Завершение запуска игры
4. **`Java_com_rockstargames_oswrapper_GameNative_implOnInitialSetup`** - JNI начальная настройка

## ⚠️ Важные замечания

### Порядок вызовов

Текущий порядок:
1. `AND_ThreadOnMain()` - инициализация Android части
2. `implOnInitialSetup()` - JNI начальная настройка (если доступна)
3. `MainThread(NULL)` - запуск главного потока
4. `AND_GameStartupDone()` - сигнал о завершении

**Если не работает, попробуйте другой порядок:**
- Сначала `implOnInitialSetup()`, потом `AND_ThreadOnMain()`
- Или убрать некоторые вызовы и оставить только `MainThread()`

### MainThread может быть блокирующим

Если `MainThread()` блокирует выполнение, возможно, нужно запускать его в отдельном потоке:

```c
#include <threads.h>

thrd_t game_thread;
if (MainThread) {
    thrd_create(&game_thread, (int(*)(void*))MainThread, NULL);
    thrd_detach(game_thread);
    // Или ждать завершения: thrd_join(game_thread, NULL);
}
```

Но сначала попробуйте текущий вариант - возможно, функция не блокирующая.

## 📝 Следующие шаги

1. ✅ Изменения в `main.c` внесены
2. ⏳ Определить структуру файлов данных Bully
3. ⏳ Адаптировать хуки в `game.c`
4. ⏳ Протестировать на Switch

## 🐛 Отладка

Если игра не запускается:

1. **Проверьте логи:**
   - Включите `DEBUG_LOG` в `config.h` (раскомментируйте `#define DEBUG_LOG 1`)
   - Смотрите сообщения `debugPrintf` в логах

2. **Проверьте ошибки:**
   - "Could not find symbol" - неправильное имя функции/переменной
   - "Could not find [файл]" - отсутствуют файлы данных
   - Крэш при запуске - возможно, неправильный порядок вызовов

3. **Попробуйте упростить:**
   - Убрать некоторые вызовы функций
   - Оставить только `MainThread()`
   - Проверить, что все символы найдены
