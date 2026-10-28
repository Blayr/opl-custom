# OPL Custom

A small fork of official Open PS2 Loader, based on upstream commit
`b3a21997` (v1.2.0 Beta, revision 2271).

## Controls

- **R3** (click the right stick): mark or unmark the selected game.
- **L3** (click the left stick): switch the current device between all games and favorites.
- Favorite games have a **gold star icon** in the left gutter, keeping game names
  aligned. The Favorites heading uses the same icon. Themes can supply `favorite.png`.
- The same controls also work on the Square/info screen.
- Game lists reserve one row of space below the All Games/Favorites heading,
  showing one fewer game while preserving the bottom margin.
- Existing launch, info, settings, refresh, paging and home/end buttons keep their roles.

Choose **Settings → Favorites storage**:

- **Game device** (default): saves `conf_favorites.cfg` in the USB/BDM library
  root (including any configured prefix), the HDD's OPL folder, or the SMB library
  folder. The file travels with the library. Copies on different devices have
  independent favorite status.
- **Memory card (shared)**: saves one master `OPL/conf_favorites.cfg` on the
  detected memory card, regardless of where OPL's global settings are stored.
  Copies of the same game ID on different devices share favorite status.

The setting applies when the Settings dialog is accepted. Use **Save Changes**
to retain this preference across restarts. Favorites themselves save immediately
in either mode. Switching storage selects the other list without merging or
overwriting files. A missing or unwritable selected store reports a save error;
it does not silently fall back to another location.

Favorites use game startup IDs, so renaming a game preserves its status. Each
device has its own filter, initially showing all games. Apps are not filtered.
All Games and Favorites remember independent selections and scroll positions
within each device for the current session. The first visit to Favorites starts
at its first entry. Refreshing/rebuilding a library resets these positions.
Existing device-local files and older memory-card master files load directly.
An older file stored beside global settings on another device can be copied to
the desired library folder or the memory card's OPL folder.

The full list remains in memory. The filter changes only which entries are
shown and navigated, preserving IDs, artwork lookups and launch/settings paths.
A failed save displays OPL's settings error and leaves the marker unchanged.
The selected favorites storage must be writable and have free space.

## Try the build

### Artwork prefetch

After the selection settles, cover (`COV`) and disc/icon (`ICO`) artwork is
loaded asynchronously for up to two visible games above and below it. This
respects the favorites filter and stops at list boundaries. These caches have
at least five slots; backgrounds are not prefetched. Custom themes using other
artwork suffixes retain their existing behavior.

Only one speculative request is outstanding, and new speculative work waits
for the IO queue to be idle. Changing selection cancels obsolete queued work;
an already-started device read finishes safely. Cached artwork remains usable.
The existing navigation delay prevents new reads during rapid scrolling.

Host validation: `python3 tests/run_prefetch.py`. On console, check slow steps,
held Up/Down, paging, favorites filtering, missing artwork and device changes.
The bundled executable includes artwork prefetching. Console validation is still pending.

The packed executable is **`dist/OPL_CUSTOM.ELF`** (about 1.3 MiB).

1. Copy it to your PS2 USB drive.
2. Launch it directly in wLaunchELF, or copy it to `mc0:/BOOT/OPL_CUSTOM.ELF`.
3. If desired, add a separate Free McBoot menu entry named **OPL Custom**.
4. Keep the working official OPL entry available while testing this first build.

No HDD reformat or artwork conversion is needed. It uses your existing OPL
configuration and game library. This is a development build, not an official
OPL release. It compiles for PS2, but has not yet been tested on a console.

Suggested console checks: mark two games, filter with L3, launch one, reboot and
verify the markers, unmark every favorite, exit the empty filter, and refresh the
HDD list. Test Up/Down, L1/R1 and L2/R2 while filtered.

## Build on Windows

The favorites save now uses its own completion flag. Background game-config
loading can no longer dismiss the save dialog early and skip the marker update.
R3 saves and updates the list immediately; Save Changes and restart are not needed.
Saving a favorite keeps the normal screen visible without a progress overlay.
Filtering preserves the selected game when possible and fills short pages from
above, so earlier favorites are not hidden off-screen unnecessarily.

The local PCSX2 development profile lives under `.tools/pcsx2/PCSX2`, separate
from normal emulator settings and memory cards. Run `./Run-Emulator.ps1` after
building to launch the uncompressed `opl.elf`. Keyboard controls: arrows to
navigate, X/C for Cross/Circle, Enter for Start, Backspace for Select, F3/F4
for L3/R3. The virtual USB disk contains non-playable menu fixtures, not games.
It includes `Scroll_Test_01` through `Scroll_Test_48`, with odd-numbered entries
marked as favorites, to exercise multiple pages and sparse filtering. To add
these fixtures again, close PCSX2 and run `python3 tests/populate_emulator.py`
in WSL. Existing favorites are preserved.

Start Docker Desktop with its Linux engine, then run:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\Build-Custom.ps1
```

The script runs the official PS2 homebrew build container, tests the production
favorites and navigation code on the host compiler, and builds the packed ELF.
The container is pinned to the image digest used for this build.

For a Linux host with a C compiler:

```sh
python3 tests/run_favorites.py
```

Tests use AddressSanitizer and UndefinedBehaviorSanitizer and exercise registry
persistence/reload, invalid keys, failed-save rollback, sparse filtering,
paging, navigation wraparound, empty lists and stable IDs. They run the actual
production filter/store code and the actual navigation functions from
`src/menusys.c`; PS2 configuration I/O is mocked.

Code is formatted with the upstream-required clang-format 12. Added strings
are appended to `lng_tmpl/_base.yml`, preserving existing language indexes.

Upstream license remains Academic Free License 3.0; see `LICENSE`.
