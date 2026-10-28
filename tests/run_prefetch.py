#!/usr/bin/env python3
"""Exercise production texture-cache scheduling with a deterministic IO worker."""
import os
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]


def without_includes(path):
    return '\n'.join(line for line in path.read_text().splitlines()
                     if not line.startswith('#include'))


stub = r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef struct { void *Mem, *Clut; int Vram, VramClut, ClutStorageMode; } GSTEXTURE;
typedef struct item_list item_list_t;
struct item_list {
    int delay;
    int (*itemGetImage)(item_list_t *, char *, int, char *, char *, GSTEXTURE *, int);
};
#define GS_CLUT_STORAGE_CSM1 0
#define GS_PSM_CT24 0
#define IO_CACHE_LOAD_ART 1
#define IO_OK 0
static int guiFrameId = 100, guiInactiveFrames = 100;
static int blocked, reads;
static void *queued;
static void (*worker)(void *);
static void texFree(GSTEXTURE *t) { free(t->Mem); t->Mem = NULL; }
static void rmUnloadTexture(GSTEXTURE *t) { (void)t; }
static void ioRegisterHandler(int type, void (*fn)(void *)) { (void)type; worker = fn; }
static int ioHasPendingRequests(void) { return queued != NULL; }
static int ioPutRequest(int type, void *data) {
    (void)type;
    if (blocked) return -1;
    assert(!queued); queued = data; return IO_OK;
}
static void drain(void) { void *q = queued; assert(q); worker(q); queued = NULL; }
static int load(item_list_t *l, char *p, int r, char *v, char *s, GSTEXTURE *t, int f) {
    (void)l; (void)p; (void)r; (void)s; (void)f;
    reads++;
    if (!strcmp(v, "missing")) return -1;
    t->Mem = malloc(1); return 0;
}
'''

checks = r'''
int main(void) {
    cacheInit();
    item_list_t list = {5, load};
    image_cache_t *c = cacheInitCache(0, "ART", 1, "COV", 5);
    int ids[6] = {-1,-1,-1,-1,-1,-1}, uids[6] = {0};
    cacheSetPrefetchWindow(c, &list, 10, 0);
    cacheGetTexture(c, &list, &ids[0], &uids[0], "current"); drain();
    for (int i = 1; i < 5; i++) {
        cachePrefetchTexture(c, &list, &ids[i], &uids[i], "neighbor");
        assert(queued); drain();
    }
    assert(reads == 5);
    for (int i = 0; i < 5; i++)
        assert(cacheGetTexture(c, &list, &ids[i], &uids[i], "cached"));
    assert(!queued);
    guiFrameId++;
    cachePrefetchTexture(c, &list, &ids[5], &uids[5], "stale");
    assert(queued);
    cacheSetPrefetchWindow(c, &list, 20, 0); drain();
    assert(reads == 5 && !prefetchPending);
    cachePrefetchTexture(c, &list, &ids[5], &uids[5], "promoted");
    cacheSetPrefetchWindow(c, &list, 21, 0);
    cacheGetTexture(c, &list, &ids[5], &uids[5], "promoted"); drain();
    assert(reads == 6 && !prefetchPending);
    guiFrameId++;
    int id = -1, uid = 0;
    cachePrefetchTexture(c, &list, &id, &uid, "cancel-on-scroll");
    guiInactiveFrames = 0; drain();
    assert(reads == 6 && !prefetchPending);
    cachePrefetchTexture(c, &list, &id, &uid, "delayed"); assert(!queued);
    guiInactiveFrames = 100;
    blocked = 1;
    cachePrefetchTexture(c, &list, &id, &uid, "blocked");
    assert(!queued && !prefetchPending && id == -1);
    blocked = 0;
    cachePrefetchTexture(c, &list, &id, &uid, "missing"); drain();
    assert(!cacheGetTexture(c, &list, &id, &uid, "missing") && id == -2);
    cachePrefetchTexture(c, &list, &id, &uid, "missing"); assert(!queued);
    cacheDestroyCache(c);
    puts("PASS: warm window, stale cancellation, demand promotion, scroll delay, blocked IO and missing art");
}
'''
with tempfile.TemporaryDirectory() as temp:
    source = Path(temp) / 'prefetch.c'
    source.write_text(stub + without_includes(root / 'include/texcache.h') +
                      without_includes(root / 'src/texcache.c') + checks)
    binary = Path(temp) / 'prefetch'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu11', '-Wall', '-Wextra',
                    '-Werror', '-fsanitize=address,undefined', '-g', str(source),
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
