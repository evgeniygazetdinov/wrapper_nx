# Исправление ошибки InitializeCriticalSection

## Проблема

При запуске игры возникает ошибка:
```
could not find symbol InitializeCriticalSection
```

## Решение

Добавлены функции Windows API в таблицу импортов в `source/imports.c`:

```c
// Windows API functions (used by some libraries)
{ "InitializeCriticalSection", (uintptr_t)&ret0 },
{ "DeleteCriticalSection", (uintptr_t)&ret0 },
{ "EnterCriticalSection", (uintptr_t)&ret0 },
{ "LeaveCriticalSection", (uintptr_t)&ret0 },
{ "TryEnterCriticalSection", (uintptr_t)&ret0 },
```

Эти функции используются некоторыми библиотеками (например, OpenAL) для синхронизации потоков. На Switch они не нужны, поэтому мы просто возвращаем 0 (успех).

## Что было изменено

Файл: `source/imports.c`

Добавлено в массив `dynlib_functions[]` после `pthread_once`:
- `InitializeCriticalSection` - инициализация критической секции
- `DeleteCriticalSection` - удаление критической секции
- `EnterCriticalSection` - вход в критическую секцию
- `LeaveCriticalSection` - выход из критической секции
- `TryEnterCriticalSection` - попытка входа в критическую секцию

## Примечание

Хук для `InitializeCriticalSection` также существует в `source/hooks/openal.c`, но он применяется позже (после `so_resolve`). Добавление в таблицу импортов гарантирует, что символ будет найден при загрузке библиотеки.

## Следующие шаги

1. Пересоберите проект:
   ```bash
   make clean
   make
   ```

2. Протестируйте на Switch

3. Если появятся другие ошибки с Windows API функциями, добавьте их аналогичным образом в `imports.c`

## Возможные дополнительные функции Windows API

Если появятся ошибки с другими функциями, возможно понадобятся:
- `GetCurrentThreadId`
- `GetCurrentProcessId`
- `Sleep`
- `GetTickCount`
- `QueryPerformanceCounter`
- И другие...

Добавляйте их по мере необходимости в `dynlib_functions[]`.
