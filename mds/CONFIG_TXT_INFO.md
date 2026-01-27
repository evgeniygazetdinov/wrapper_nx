# Информация о config.txt

## 📄 Что такое config.txt?

`config.txt` - это файл конфигурации для wrapper Bully на Nintendo Switch. Он позволяет настраивать различные параметры игры.

## 🔄 Как создается config.txt?

**Файл создается автоматически** при первом запуске игры, если его нет!

В коде (`source/main.c`, строки 120-121):
```c
if (read_config(CONFIG_NAME) < 0)
    write_config(CONFIG_NAME);
```

Если файл не найден, он создается с настройками по умолчанию.

## 📍 Где должен находиться config.txt?

Файл должен находиться **в той же папке, что и .nro**. При старте приложение переходит в каталог с .nro (если загрузчик передаёт путь в argv[0]), затем читает оттуда `config.txt`. Поэтому `data_path` из этого файла учитывается.

Например:
```
sdmc:/switch/bully/
├── wrapper.nro
├── libGame.so
├── config.txt          ← здесь
├── bullyorig/
└── bully/
```
Если .nro лежит в `sdmc:/switch/bully/`, а рабочим каталогом при запуске был `sdmc:/switch`, конфиг раньше читался не из папки игры. Сейчас сначала выполняется переход в каталог с .nro, затем чтение config.txt — поэтому `data_path` из правильного config работает.

## ⚙️ Параметры конфигурации

### Разрешение экрана
```
screen_width -1    # -1 = автоматически (1920x1080 в доке, 1280x720 в портативном)
screen_height -1   # Или укажите конкретные значения
```

### Графика
```
use_bloom 0              # 0 = выключено, 1 = включено
trilinear_filter 1       # Трилинейная фильтрация текстур
disable_mipmaps 0        # 0 = использовать mipmaps, 1 = отключить
```

### Язык
```
language 0    # 0 = английский, 1-6 = другие языки
```

### Детализация
```
character_shadows 1      # Тени персонажей (1 = одна тень, 2 = тени ног)
drop_highest_lod 0       # Отключить самый высокий LOD
decal_limit 0.5          # Лимит декалей
debris_limit 1.0         # Лимит обломков
```

### Управление (из Max Payne, могут не применяться к Bully)
```
crouch_toggle 1          # Переключение приседания
show_weapon_menu 0       # Показывать меню оружия
```

### Моды
```
mod_file                 # Путь к файлу мода (оставьте пустым, если не используете)
```

### Путь к данным (решение «could not find bullyorig»)
```
data_path                # Каталог с bullyorig/ и bully/ (опционально)
```
Если при запуске выдаётся «could not find bullyorig» при том, что папка есть, значит рабочая директория приложения — не та, где лежат данные. Варианты:
1. Запускать .nro из папки, где лежат bullyorig, bully, libGame.so (так текущая директория будет правильной).
2. В config.txt указать полный путь: `data_path sdmc:/switch/bully` (подставьте свой путь к папке с bullyorig). Приложение перейдёт в этот каталог перед проверкой данных.

## 📝 Пример файла config.txt

Смотрите файл `config.txt.example` в корне проекта для примера.

Или используйте этот минимальный вариант:
```
screen_width -1
screen_height -1
use_bloom 0
trilinear_filter 1
disable_mipmaps 0
language 0
crouch_toggle 1
show_weapon_menu 0
character_shadows 1
drop_highest_lod 0
decal_limit 0.5
debris_limit 1.0
mod_file 
```

## ✅ Что делать?

**Ничего!** Просто запустите игру, и `config.txt` создастся автоматически с настройками по умолчанию.

Если хотите настроить параметры заранее:
1. Скопируйте `config.txt.example` в папку игры как `config.txt`
2. Или создайте файл вручную с нужными параметрами
3. Или просто запустите игру - файл создастся сам, потом отредактируйте его

## 🔍 Где найти настройки по умолчанию?

Настройки по умолчанию определены в `source/config.c`, функция `read_config()` (строки 50-62):
- `screen_width = -1` (авто)
- `screen_height = -1` (авто)
- `use_bloom = 0`
- `trilinear_filter = 1`
- `disable_mipmaps = 0`
- `language = 0` (английский)
- `crouch_toggle = 1`
- `character_shadows = 1`
- `drop_highest_lod = 0`
- `show_weapon_menu = 0`
- `decal_limit = 0.5`
- `debris_limit = 1.0`
