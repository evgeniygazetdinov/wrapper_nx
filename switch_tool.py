#!/usr/bin/env python3
"""
Один скрипт для деплоя .nro на Switch и запуска nxlink (логи).

Деплой:
  python switch_tool.py deploy                    # MTP: копирование в SWITCH_SD_PATH или путь по умолчанию
  python switch_tool.py deploy --ftp 192.168.0.113   # FTP на Switch (на консоли запусти FTPD)
  python switch_tool.py deploy --mtp /path/to/sd/switch

Nxlink (отправка .nro + приём логов через Docker):
  python switch_tool.py nxlink [IP]              # IP опционален (есть дефолт)
  python switch_tool.py nxlink 192.168.0.113 listen
  На Switch: в Homebrew Menu нажми Y (netloader), затем запусти скрипт.

Сначала деплой, потом nxlink:
  python switch_tool.py 192.168.0.113            # deploy по FTP на IP, затем nxlink на IP
  python switch_tool.py --no-deploy 192.168.0.113 # только nxlink (без деплоя)
"""

import argparse
import os
import subprocess
import sys
from typing import Optional

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
DEFAULT_NRO = "project.nro"
DEFAULT_FTP_PORT = 5000
DEFAULT_REMOTE_PATH = "/switch"
DEFAULT_SWITCH_IP = "192.168.0.113"
DEFAULT_MTP_PATH = "/run/user/1000/gvfs/mtp:host=-_DBI_XJE10002145012/1: SD Card/switch"


def find_nro():
    os.chdir(SCRIPT_DIR)
    if os.path.isfile(DEFAULT_NRO):
        return DEFAULT_NRO
    for f in os.listdir("."):
        if f.endswith(".nro"):
            return f
    return None


def deploy_ftp(nro: str, ip: str, port: int = DEFAULT_FTP_PORT) -> bool:
    remote = f"{DEFAULT_REMOTE_PATH}/{os.path.basename(nro)}"
    url = f"ftp://{ip}:{port}{remote}"
    print(f"Отправка по FTP на {ip}:{port} → {remote}")
    try:
        subprocess.run(
            ["curl", "-sS", "-T", nro, url, "--user", "anonymous:"],
            check=True,
            cwd=SCRIPT_DIR,
        )
        print(f"Готово: {nro} → {url}")
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("Ошибка FTP. Проверь: на Switch запущен FTPD (Homebrew), ПК и Switch в одной Wi‑Fi, IP верный.", file=sys.stderr)
        return False


def deploy_mtp(nro: str, dest_dir: str) -> bool:
    dest_dir = dest_dir.rstrip("/") + "/"
    if not os.path.isdir(dest_dir):
        print(f"Папка не найдена: {dest_dir}", file=sys.stderr)
        print("MTP: подключи Switch по USB (DBI → USB Install → MTP).", file=sys.stderr)
        print("FTP: switch_tool.py deploy --ftp <IP>", file=sys.stderr)
        return False
    dest = os.path.join(dest_dir, os.path.basename(nro))
    try:
        import shutil
        shutil.copy2(nro, dest)
        print(f"Готово: {nro} → {dest}")
        return True
    except OSError:
        print("Ошибка копирования (I/O). Закрой NRO на Switch или переподключи USB.", file=sys.stderr)
        return False


def run_nxlink(nro: str, ip: Optional[str], listen: bool) -> None:
    size_mb = os.path.getsize(nro) // (1024 * 1024)
    print(f"NRO: {nro} ({size_mb} MB). Отправка по Wi‑Fi может занять 1–2 мин.")
    print("На Switch: сначала нажми Y (netloader) в Homebrew Menu, потом запускай скрипт.")
    print()

    if listen and ip:
        cmd = [
            "docker", "run", "--rm", "-it", "--network", "host",
            "-v", f"{SCRIPT_DIR}:/project", "-w", "/project",
            "devkitpro/devkita64",
            "/opt/devkitpro/tools/bin/nxlink", nro, "-a", ip, "-s",
        ]
    elif ip:
        cmd = [
            "docker", "run", "--rm", "-it", "--network", "host",
            "-v", f"{SCRIPT_DIR}:/project", "-w", "/project",
            "devkitpro/devkita64",
            "/opt/devkitpro/tools/bin/nxlink", "-a", ip, "-s", nro,
        ]
    else:
        cmd = [
            "docker", "run", "--rm", "-it", "--network", "host",
            "-v", f"{SCRIPT_DIR}:/project", "-w", "/project",
            "devkitpro/devkita64",
            "/opt/devkitpro/tools/bin/nxlink", "-s", nro,
        ]

    try:
        subprocess.run(cmd, cwd=SCRIPT_DIR)
    except FileNotFoundError:
        print("Docker не найден. Установи Docker и devkitPro образ.", file=sys.stderr)
        sys.exit(1)


def main():
    parser = argparse.ArgumentParser(
        description="Деплой .nro на Switch и/или запуск nxlink (логи).",
        epilog=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "ip_or_path",
        nargs="?",
        default=None,
        help="IP Switch или путь MTP к папке SD/switch",
    )
    parser.add_argument(
        "--no-deploy",
        action="store_true",
        help="Не делать деплой, только nxlink",
    )
    sub = parser.add_subparsers(dest="command", help="Подкоманда")

    deploy_p = sub.add_parser("deploy", help="Только деплой .nro")
    deploy_p.add_argument("--ftp", metavar="IP", help="Деплой по FTP на указанный IP")
    deploy_p.add_argument("--mtp", metavar="PATH", help="Путь к папке SD/switch (MTP)")

    nxlink_p = sub.add_parser("nxlink", help="Только nxlink (отправка + логи)")
    nxlink_p.add_argument("ip", nargs="?", default=None, help="IP Switch")
    nxlink_p.add_argument("listen", nargs="?", choices=["listen"], help='Режим listen')

    args = parser.parse_args()

    nro = find_nro()
    if not nro:
        print("Не найден .nro в", SCRIPT_DIR, ". Сначала собери: make", file=sys.stderr)
        sys.exit(1)

    if args.command == "deploy":
        if args.ftp:
            ok = deploy_ftp(nro, args.ftp)
        elif args.mtp:
            ok = deploy_mtp(nro, args.mtp)
        else:
            ip = os.environ.get("SWITCH_IP", args.ip_or_path)
            if ip and not os.path.isdir(ip):
                ok = deploy_ftp(nro, ip, int(os.environ.get("SWITCH_FTP_PORT", DEFAULT_FTP_PORT)))
            else:
                path = os.environ.get("SWITCH_SD_PATH", args.ip_or_path or DEFAULT_MTP_PATH)
                ok = deploy_mtp(nro, path)
        sys.exit(0 if ok else 1)

    if args.command == "nxlink":
        ip = args.ip or os.environ.get("SWITCH_IP", DEFAULT_SWITCH_IP)
        listen = args.listen == "listen"
        run_nxlink(nro, ip, listen)
        return

    # Режим по умолчанию: deploy затем nxlink
    ip = args.ip_or_path or os.environ.get("SWITCH_IP", DEFAULT_SWITCH_IP)
    if not args.no_deploy:
        if ip and not os.path.isdir(ip):
            if not deploy_ftp(nro, ip, int(os.environ.get("SWITCH_FTP_PORT", DEFAULT_FTP_PORT))):
                sys.exit(1)
        else:
            path = os.environ.get("SWITCH_SD_PATH", args.ip_or_path or DEFAULT_MTP_PATH)
            if not deploy_mtp(nro, path):
                sys.exit(1)
    nxlink_ip = ip if (ip and not os.path.isdir(ip)) else os.environ.get("SWITCH_IP", DEFAULT_SWITCH_IP)
    run_nxlink(nro, nxlink_ip, listen=False)


if __name__ == "__main__":
    main()
