#include "include/opl.h"
#include "include/config.h"
#include "include/favorites.h"
#include "include/iosupport.h"
#include <string.h>

static config_set_t *favorites[MODE_COUNT];
static config_set_t *masterFavorites;

void favoritesEnd(void)
{
    for (int mode = 0; mode < MODE_COUNT; mode++)
        favoritesUnload(mode);
    if (masterFavorites) {
        configFree(masterFavorites);
        masterFavorites = NULL;
    }
}

void favoritesUnload(int mode)
{
    if (mode >= 0 && mode < MODE_COUNT && favorites[mode]) {
        configFree(favorites[mode]);
        favorites[mode] = NULL;
    }
}

void favoritesLoad(int mode, const char *prefix)
{
    if (mode < 0 || mode >= MODE_COUNT)
        return;
    favoritesUnload(mode);
    if (mode == APP_MODE)
        return;

    if (gFavoritesStorage == FAVORITES_STORAGE_MC) {
        if (!masterFavorites) {
            char path[256];
            snprintf(path, sizeof(path), "%s/conf_favorites.cfg", gBaseMCDir);
            masterFavorites = configAlloc(0, NULL, path);
            configRead(masterFavorites);
        }
        return;
    }
    if (!prefix || !prefix[0])
        return;

    char path[256];
    int length = strlen(prefix);
    const char *separator = prefix[length - 1] == '/' || prefix[length - 1] == ':' ? "" : "/";
    if (snprintf(path, sizeof(path), "%s%sconf_favorites.cfg", prefix, separator) >= (int)sizeof(path))
        return;
    favorites[mode] = configAlloc(0, NULL, path);
    configRead(favorites[mode]);
}

static int favoritesValidKey(const char *startup)
{
    return startup && startup[0] && startup[0] != '#' &&
           strlen(startup) < CONFIG_KEY_NAME_LEN && !strchr(startup, '=') &&
           !strchr(startup, '\n') && !strchr(startup, '\r');
}

static config_set_t *favoritesConfig(int mode)
{
    if (mode < 0 || mode >= MODE_COUNT || mode == APP_MODE)
        return NULL;
    return gFavoritesStorage == FAVORITES_STORAGE_MC ? masterFavorites : favorites[mode];
}

int favoritesContains(int mode, const char *startup)
{
    int favorite = 0;
    config_set_t *config = favoritesConfig(mode);
    if (config && favoritesValidKey(startup))
        configGetInt(config, startup, &favorite);
    return favorite != 0;
}

int favoritesSet(int mode, const char *startup, int favorite)
{
    config_set_t *config = favoritesConfig(mode);
    if (!config || !favoritesValidKey(startup))
        return 0;

    const int previous = favoritesContains(mode, startup);
    if (previous == !!favorite)
        return 1;

    if (favorite)
        configSetInt(config, startup, 1);
    else
        configRemoveKey(config, startup);

    if (configWrite(config))
        return 1;

    // Do not show a successful toggle when storage is unavailable or full.
    if (previous)
        configSetInt(config, startup, 1);
    else
        configRemoveKey(config, startup);
    return 0;
}
