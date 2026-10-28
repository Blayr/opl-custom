#!/usr/bin/env python3
"""Run host-side tests against the production favorites and navigation code."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/menusys.c').read_text()
start = source.index('static void menuFirstPage()')
end = source.index('void menuSetSelectedItem', start)
with tempfile.TemporaryDirectory() as temp:
    work = Path(temp)
    (work / 'navigation.inc').write_text(source[start:end])
    (work / 'favorite_action.inc').write_text(source[source.index('static char favoriteStartup'):source.index('static void menuToggleFavoritesFilter')])
    binary = work / 'favorites_test'
    subprocess.run([
        os.environ.get('CC', 'cc'), '-std=gnu11', '-Wall', '-Wextra', '-Werror',
        '-fsanitize=address,undefined', '-g',
        '-I' + str(root / 'tests/stubs'), '-I' + str(root), '-I' + temp,
        '-include', str(root / 'tests/stubs/include/opl.h'),
        str(root / 'src/favorites.c'), str(root / 'src/menufilter.c'),
        str(root / 'tests/favorites_test.c'), '-o', str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
