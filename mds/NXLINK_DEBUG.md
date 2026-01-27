# nxlink — вывод логов на ПК при отладке на Switch

**nxlink** — утилита devkitPro для загрузки .nro по сети и приёма stdout/stderr с консоли на компьютер. Так в терминале ПК виден весь вывод `debugPrintf` и `printf` из приложения.

## 1. Включить отладочный вывод в проекте

В `source/config.h` раскомментируйте:

```c
#define DEBUG_LOG 1
```

При `DEBUG_LOG`:
- вызывается `nxlinkStdio()` при старте — stdout/stderr уходят по сети в nxlink;
- `debugPrintf()` пишет в `debug.log` на SD и в stdout (то есть в nxlink).

Пересоберите проект:

```bash
make clean
make
```

Будет собран `project.nro` (или имя из Makefile).

## 2. Подготовка Switch

1. **Один компьютер и одна консоль в одной подсети**  
   Switch и ПК должны быть в одной Wi‑Fi сети.

2. **Запуск Homebrew Menu**  
   Запустите Homebrew Menu (hbmenu) на Switch.

3. **Включение netloader**  
   В Homebrew Menu нажмите **Y** (или свайп вниз).  
   Должно появиться что‑то вроде «Netloader active» / «Waiting for connection».

4. **IP Switch (если нужен)**  
   В настройках консоли: **Internet → Connection Status** — там будет IP, например `192.168.1.100`.  
   Он понадобится, если `nxlink` не увидит консоль по broadcast.

## 3. Запуск nxlink на ПК

**Команда «nxlink» не найдена?** — nxlink не из apt, а из devkitPro. Варианты:

### Вариант «только Docker» (сборка через docker_make.sh)

nxlink есть внутри образа `devkitpro/devkita64`. Запуск из папки проекта:

```bash
cd /home/ev/my_code/wrapper_nx

# По broadcast (Switch в той же сети):
docker run --rm -it --network host \
  -v "$(pwd):/project" -w /project \
  devkitpro/devkita64 \
  /opt/devkitpro/tools/bin/nxlink -s project.nro

# С указанием IP Switch:
docker run --rm -it --network host \
  -v "$(pwd):/project" -w /project \
  devkitpro/devkita64 \
  /opt/devkitpro/tools/bin/nxlink -a 192.168.1.XXX -s project.nro
```

Подставьте свой IP вместо `192.168.1.XXX`. На Switch предварительно нажмите **Y** в Homebrew Menu (netloader).

**Скрипт-обёртка в проекте:**

```bash
cd /home/ev/my_code/wrapper_nx
chmod +x nxlink_debug.sh

./nxlink_debug.sh              # поиск Switch по broadcast
./nxlink_debug.sh 192.168.1.5  # с указанием IP Switch
```

### Вариант «devkitPro установлен в системе»

Если ставили devkitPro локально (не только Docker):

```bash
# Полный путь:
$DEVKITPRO/tools/bin/nxlink -s project.nro

# Или добавьте в PATH и вызывайте nxlink:
export PATH="$DEVKITPRO/tools/bin:$PATH"
nxlink -s project.nro
```

Типичный путь при установке через pacman: `/opt/devkitpro/tools/bin/nxlink`.

---

### Вариант A: консоль в той же подсети (broadcast)

```bash
nxlink -s /path/to/project.nro
```

Флаг **`-s`** = после отправки .nro **запустить сервер** и принимать stdout/stderr с консоли.  
Всё, что приложение пишет в stdout (в т.ч. через `debugPrintf`), будет выводиться в этом терминале.

### Вариант B: указать IP Switch

Если пишет «No response from Switch!»:

```bash
nxlink -a 192.168.1.100 -s /path/to/project.nro
```

`192.168.1.100` замените на реальный IP вашей консоли.

### Вариант C: из папки сборки

```bash
cd /home/ev/my_code/wrapper_nx
nxlink -s project.nro
# или с IP:
nxlink -a 192.168.1.100 -s project.nro
```

## 4. Что вы увидите

1. nxlink находит Switch (или подключается по `-a`).
2. Шлёт .nro на консоль и запускает его.
3. Печатает что‑то вроде `Sending project.nro, ... bytes` и `starting server`.
4. Дальше в этом же терминале идут строки из приложения:
   - `[main] patch_openal`
   - `[main] patch_opengl`
   - `[main] patch_game`
   - `[patch_game] start`
   - `[patch_game] done`
   - `[main] patches done`
   - и т.д.

По **последней выведенной строке** можно понять, где происходит краш.

## 5. Полезные флаги nxlink

| Флаг | Описание |
|------|----------|
| `-s` | После загрузки .nro запустить сервер и показывать stdout/stderr с консоли |
| `-a <IP>` | IP или hostname Switch (если broadcast не срабатывает) |
| `-r <N>` | Число попыток поиска Switch (по умолчанию 10) |
| `-p <path>` | Путь на SD, куда класть .nro (по умолчанию — корень) |
| `-h` | Справка |

Пример с IP и путём:

```bash
nxlink -a 192.168.1.100 -s -p /switch/project.nro
```

## 6. Типичные проблемы

**«No response from Switch!»**  
- На Switch нажат **Y** в Homebrew Menu (netloader включён).  
- Switch и ПК в одной сети.  
- Используйте `-a <IP_Switch>`.

**Вывод не появляется в nxlink**  
- В `config.h` включён `#define DEBUG_LOG 1` и проект пересобран.  
- Запускаете именно `nxlink -s project.nro` (флаг `-s` обязателен для приёма логов).  
- Приложение реально стартует (не падает до `userAppInit` / до `nxlinkStdio()`).

**«Connection refused» / обрывы**  
- Проверьте firewall на ПК (должен разрешать входящие на порт, который слушает nxlink после `-s`).  
- Порт 28771 (client port) используется для приёма stdout; не блокируйте его.

## 7. Логи на SD (без nxlink)

Даже без nxlink при `DEBUG_LOG 1` все вызовы `debugPrintf` пишутся в файл `debug.log` на SD карте консоли (имя задаётся `LOG_NAME` в `config.h`).  
Путь зависит от того, откуда запускается .nro (обычно корень выбранной приложением директории или текущая папка Homebrew Menu).

Чтобы смотреть логи на ПК в реальном времени, удобнее всего пользоваться **nxlink -s** так, как описано выше.
