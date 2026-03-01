#!/usr/bin/env python3
"""
Сборка wrapper_nx через Docker (devkitPro devkita64).
Использование: python docker_make.py [цель make...]
"""

import os
import glob
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
IMAGE = "devkitpro/devkita64"


def main():
    os.chdir(SCRIPT_DIR)

    try:
        subprocess.run(
            ["docker", "info"],
            capture_output=True,
            check=True,
        )
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("Ошибка: проверьте Docker (sudo docker info)", file=sys.stderr)
        sys.exit(1)

    make_targets = sys.argv[1:] if len(sys.argv) > 1 else []
    cmd = ["docker", "run", "--rm", "-v", f"{SCRIPT_DIR}:/project", "-w", "/project", IMAGE, "make"] + make_targets

    print("Сборка wrapper_nx...")
    rc = subprocess.run(cmd)
    if rc.returncode != 0:
        sys.exit(rc.returncode)

    print("Готово! Проверьте файлы:")
    for ext in ("nro", "nsp", "elf"):
        for path in sorted(glob.glob(os.path.join(SCRIPT_DIR, f"*.{ext}"))):
            if os.path.isfile(path):
                print(f"  {os.path.basename(path)}  {os.stat(path).st_size} bytes")
    listed = any(
        os.path.isfile(p)
        for ext in ("nro", "nsp", "elf")
        for p in glob.glob(os.path.join(SCRIPT_DIR, f"*.{ext}"))
    )
    if not listed:
        print("  Бинарные файлы не найдены")


if __name__ == "__main__":
    main()
