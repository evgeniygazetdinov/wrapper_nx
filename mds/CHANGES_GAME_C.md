# Изменения в game.c для Bully

## ✅ Внесенные изменения

### 1. Удалены специфичные для Max Payne функции

- Удалена структура `MaxPayne_InputControl`
- Удалена функция `MaxPayne_ConfiguredInput_readCrouch` (переключение приседания)
- Удален хук для `WeaponSwiper::Draw` (меню оружия)

### 2. Добавлены проверки существования символов

Все хуки теперь проверяют существование символов перед установкой:
```c
if (so_find_addr("symbol_name") != 0)
    hook_arm64(so_find_addr("symbol_name"), (uintptr_t)function);
```

Это предотвращает ошибки, если символы не существуют в Bully.

### 3. Обновлены хуки экрана

Добавлена поддержка альтернативных функций экрана:
- `_Z17OS_ScreenGetWidthv` / `_Z18OS_ScreenGetHeightv` (как в Max Payne)
- `_Z14GetScreenWidthv` / `_Z15GetScreenHeightv` (альтернативные функции Bully)

### 4. Обновлены хуки управления

- Оставлены хуки `WarGamepad_*` (если существуют)
- Добавлены TODO комментарии для Bully-специфичных функций:
  - `Java_com_rockstargames_oswrapper_GameNative_implOnGamepadButtonDown`
  - `Java_com_rockstargames_oswrapper_GameNative_implOnGamepadButtonUp`
  - `Java_com_rockstargames_oswrapper_GameNative_implOnGamepadAxesChanged`
  - `_Z16OS_GamepadButtonjj`
  - `_Z14OS_GamepadAxisjj`

### 5. Обновлены переменные устройства

Добавлены проверки для:
- `deviceChip`
- `deviceForm`
- `definedDevice`

## 🔍 Найденные функции Bully

### Функции экрана:
- `_Z17OS_ScreenGetWidthv` ✅
- `_Z18OS_ScreenGetHeightv` ✅
- `_Z14GetScreenWidthv` ✅
- `_Z15GetScreenHeightv` ✅

### Функции управления:
- `Java_com_rockstargames_oswrapper_GameNative_implOnGamepadButtonDown` (JNI)
- `Java_com_rockstargames_oswrapper_GameNative_implOnGamepadButtonUp` (JNI)
- `Java_com_rockstargames_oswrapper_GameNative_implOnGamepadAxesChanged` (JNI)
- `_Z16OS_GamepadButtonjj` (прямая функция)
- `_Z14OS_GamepadAxisjj` (прямая функция)
- `_Z15LIB_InputUpdatei` (обновление ввода)
- `_Z16LIB_GamepadStateii` (состояние геймпада)

### Функции файлов:
- `_Z11OS_FileOpen14OSFileDataAreaPPvPKc16OSFileAccessType`
- `_Z11OS_FileReadPvS_i`
- `_Z11OS_FileSizePv`

## ⚠️ Что еще нужно сделать

### 1. Реализовать обработчики JNI функций управления

Если Bully использует JNI функции для управления, нужно:
- Либо эмулировать JNI окружение
- Либо найти внутренние функции, которые вызываются из JNI
- Либо перехватывать JNI вызовы и преобразовывать их в наши функции

### 2. Адаптировать функции файловой системы

Если Bully использует другие функции файловой системы:
- Проверить, используются ли `OS_FileOpen`, `OS_FileRead`, `OS_FileSize`
- Создать хуки для них, если нужно

### 3. Протестировать на Switch

После компиляции нужно протестировать:
- Запускается ли игра
- Работает ли управление
- Работает ли экран
- Есть ли ошибки в логах

## 📝 Следующие шаги

1. ✅ Удалены специфичные для Max Payne функции
2. ✅ Добавлены проверки существования символов
3. ✅ Обновлены хуки экрана
4. ⏳ Реализовать обработчики управления (если нужно)
5. ⏳ Адаптировать файловую систему (если нужно)
6. ⏳ Протестировать на Switch

## 🐛 Отладка

Если управление не работает:

1. Проверьте, вызываются ли функции `WarGamepad_*`
2. Если нет, попробуйте найти, какие функции используются
3. Возможно, нужно реализовать обработчики для JNI функций
4. Или найти прямые функции управления и захукать их

Если экран не работает:

1. Проверьте, вызываются ли функции `OS_ScreenGet*` или `GetScreen*`
2. Проверьте логи - возможно, функции не найдены
3. Попробуйте найти другие функции экрана в библиотеке
