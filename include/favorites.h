#ifndef __FAVORITES_H
#define __FAVORITES_H

#define FAVORITES_STORAGE_DEVICE 0
#define FAVORITES_STORAGE_MC     1

void favoritesEnd(void);

// Load a library's favorites on the IO worker after its device is mounted.
void favoritesLoad(int mode, const char *prefix);
void favoritesUnload(int mode);
// Favorites are keyed by startup ID within this device/library only.
int favoritesContains(int mode, const char *startup);
// Returns nonzero only when the new state was saved; restores memory on failure.
int favoritesSet(int mode, const char *startup, int favorite);

#endif
