# Инструкция по установке Bully на Nintendo Switch

## 📁 Структура папок на SD карте

### Путь установки
```
/sdcard/switch/bully/
```

Или на Windows/Mac при подключении SD карты:
```
SD:/switch/bully/
```

## 📦 Полная структура файлов

```
/switch/bully/
├── project.nro              # Скомпилированный wrapper (главный файл)
├── libGame.so               # Библиотека игры (из split_config.arm64_v8a.apk)
├── config.txt               # Файл конфигурации (создается автоматически)
├── debug.log                # Лог файл (создается при запуске)
│
├── bullyorig/               # Основные данные игры (из data_0.zip)
│   ├── version.cfg
│   ├── runtime.xml
│   ├── memory.xml
│   ├── jumptable.bin
│   ├── bs-checkpoint.dat
│   ├── config/
│   │   ├── dat/
│   │   ├── dat2/
│   │   └── ...
│   ├── models/
│   ├── audio/
│   │   ├── Banks/
│   │   └── CONFIG/
│   ├── anim/
│   ├── objects/
│   ├── data/
│   ├── scripts/
│   ├── stream/
│   ├── coll/
│   ├── cuts/
│   ├── act/
│   └── lawnplacment/
│
├── bully/                   # Дополнительные данные (из data_1.zip)
│   ├── speech_*.snd         # Файлы речи персонажей
│   ├── mobile_wfade*.tex    # Текстуры загрузки
│   ├── english_background.tex
│   ├── font_japanese.tex
│   ├── mx_*.snd             # Музыка
│   └── *.msh                # Модели
│
└── savegames/               # Папка для сохранений (создается автоматически)
    └── ...
```

## 📝 Пошаговая инструкция по установке

### Шаг 1: Подготовка файлов

1. **Скомпилируйте проект:**
   ```bash
   cd /home/ev/my_code/wrapper_nx
   make clean
   make
   ```
   Это создаст `project.nro` в корне проекта.

2. **Скопируйте libGame.so:**
   ```bash
   cp /home/ev/Downloads/bully_archive/Bully_v1.4.277.build.3793806/extracted_libs/libGame.so ./
   ```

3. **Извлеките данные игры:**
   ```bash
   ./extract_bully_data.sh ./bully_game_data
   ```
   Это создаст папки `bullyorig/` и `bully/` в `./bully_game_data/`

### Шаг 2: Копирование на SD карту

1. **Подключите SD карту Switch к компьютеру** (через кардридер или USB)

2. **Создайте папку на SD карте:**
   ```
   SD:/switch/bully/
   ```

3. **Скопируйте файлы:**
   ```bash
   # Основные файлы
   cp project.nro /path/to/sd/switch/bully/
   cp libGame.so /path/to/sd/switch/bully/
   
   # Данные игры
   cp -r bully_game_data/bullyorig /path/to/sd/switch/bully/
   cp -r bully_game_data/bully /path/to/sd/switch/bully/
   
   # Опционально: config.txt (если хотите настроить заранее)
   cp config.txt /path/to/sd/switch/bully/
   ```

### Шаг 3: Запуск на Switch

1. **Включите Switch с CFW** (Custom Firmware)

2. **Запустите Homebrew Menu** (hbmenu)

3. **Найдите и запустите "bully"** (или имя, которое вы указали в Makefile)

4. **При первом запуске:**
   - Игра создаст `config.txt` автоматически
   - Игра создаст папку `savegames/` автоматически
   - Могут появиться логи в `debug.log`

## ⚠️ Важные замечания

### Размер данных
- **libGame.so**: ~19MB
- **bullyorig/**: ~168MB (из data_0.zip)
- **bully/**: ~825MB (из data_1.zip)
- **Итого**: ~1GB

Убедитесь, что на SD карте достаточно места!

### Права доступа
Убедитесь, что все файлы имеют права на чтение:
```bash
chmod -R 755 /path/to/sd/switch/bully/
```

### Пути в коде
В коде используется относительный путь `.` (текущая директория), поэтому:
- Все файлы должны быть в `/switch/bully/`
- Не используйте вложенные папки
- `StorageRootPath` установлен в `"."` (текущая папка)

### Проверка структуры
Перед запуском убедитесь, что структура правильная:
```bash
# На Switch (через FTP или терминал)
ls -la /switch/bully/
# Должны быть видны:
# - project.nro
# - libGame.so
# - bullyorig/ (папка)
# - bully/ (папка)
```

## 🔧 Альтернативные пути

Если хотите использовать другое имя папки:

1. **Измените APP_TITLE в Makefile:**
   ```makefile
   APP_TITLE := Bully
   ```

2. **Или просто переименуйте папку:**
   ```
   /switch/bully/  →  /switch/bully_game/
   ```

3. **Важно:** Путь к данным в коде не изменится, так как используется относительный путь.

## 📋 Чеклист перед запуском

- [ ] `project.nro` скопирован в `/switch/bully/`
- [ ] `libGame.so` скопирован в `/switch/bully/`
- [ ] Папка `bullyorig/` скопирована в `/switch/bully/`
- [ ] Папка `bully/` скопирована в `/switch/bully/`
- [ ] Проверены права доступа на файлы
- [ ] На SD карте достаточно места (~1GB)
- [ ] Switch запущен с CFW
- [ ] Homebrew Menu доступен

## 🐛 Отладка

Если игра не запускается:

1. **Проверьте логи:**
   ```bash
   cat /switch/bully/debug.log
   ```

2. **Проверьте структуру:**
   ```bash
   ls -la /switch/bully/
   ls -la /switch/bully/bullyorig/
   ls -la /switch/bully/bully/
   ```

3. **Проверьте ошибки:**
   - "Could not find [файл]" → проверьте, что файлы скопированы
   - "Could not load libGame.so" → проверьте путь и права доступа
   - "Could not find symbol" → неправильная версия .so файла

## 📞 Дополнительная информация

- Все пути в коде относительные (от текущей директории)
- Файлы создаются автоматически: `config.txt`, `savegames/`, `debug.log`
- Размер данных большой (~1GB), убедитесь в наличии места
- Используйте CFW для доступа к необходимым системным вызовам
