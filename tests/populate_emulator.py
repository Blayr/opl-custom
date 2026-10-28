#!/usr/bin/env python3
"""Add numbered, non-playable menu fixtures to the local emulator USB disk.

Run in WSL with mtools installed and PCSX2 closed. Existing favorites are kept.
"""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
image = root / '.tools/opl-test-usb-partitioned.img'
fixture = root / '.tools/ui-fixture.iso'
disk = str(image) + '@@1048576'
if not image.is_file() or not fixture.is_file():
    raise SystemExit('Configure the local emulator test disk first.')

with tempfile.TemporaryDirectory() as temp:
    favorites = subprocess.run(
        ['mtype', '-i', disk, '::conf_favorites.cfg'],
        capture_output=True, text=True)
    entries = dict(line.split('=', 1) for line in favorites.stdout.splitlines()
                   if '=' in line)
    for number in range(1, 49):
        startup = f'SLUS_100.{number:02d}'
        name = f'::DVD/{startup}.Scroll_Test_{number:02d}.iso'
        subprocess.run(['mcopy', '-o', '-i', disk, str(fixture), name], check=True)
        if number % 2:
            entries[startup] = '1'
    config = Path(temp) / 'conf_favorites.cfg'
    config.write_text(''.join(f'{key}={value}\n' for key, value in entries.items()))
    subprocess.run(['mcopy', '-o', '-i', disk, str(config), '::conf_favorites.cfg'], check=True)
print('Added Scroll_Test_01 through Scroll_Test_48; odd-numbered entries are favorites.')
