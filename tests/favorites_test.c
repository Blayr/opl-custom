#include "include/opl.h"
#include "include/config.h"
#include "include/favorites.h"
#include "include/menusys.h"
#include <assert.h>

static config_set_t registry;
int gFavoritesStorage;
char *gBaseMCDir = "mc?:OPL";
static int canSave = 1, writes;
static char saved[4096];
static char persisted[8][4096];
static char paths[8][256];
static int fileIndex(const char *path)
{
    for (int i = 0; i < 8; i++) {
        if (!paths[i][0])
            snprintf(paths[i], sizeof(paths[i]), "%s", path);
        if (!strcmp(paths[i], path))
            return i;
    }
    assert(0);
    return -1;
}
config_set_t *configAlloc(int type, config_set_t *config, char *filename)
{
    (void)type;
    (void)config;
    config = !strcmp(filename, "mass0:conf_favorites.cfg") ? &registry : calloc(1, sizeof(*config));
    memset(config, 0, sizeof(*config));
    config->filename = strdup(filename);
    return config;
}
int configGetInt(config_set_t *config, const char *key, int *value)
{
    for (struct config_value_t *it = config->head; it; it = it->next)
        if (!strcmp(it->key, key)) {
            *value = atoi(it->val);
            return 1;
        }
    return 0;
}
int configSetInt(config_set_t *config, const char *key, int value)
{
    struct config_value_t *it = config->head;
    while (it && strcmp(it->key, key))
        it = it->next;
    if (!it) {
        it = calloc(1, sizeof(*it));
        snprintf(it->key, sizeof(it->key), "%s", key);
        it->next = config->head;
        config->head = it;
    }
    snprintf(it->val, sizeof(it->val), "%d", value);
    config->modified = 1;
    return 1;
}
int configRemoveKey(config_set_t *config, const char *key)
{
    struct config_value_t **it = &config->head;
    while (*it) {
        if (!strcmp((*it)->key, key)) {
            struct config_value_t *old = *it;
            *it = old->next;
            free(old);
            config->modified = 1;
            return 1;
        }
        it = &(*it)->next;
    }
    return 0;
}
int configWrite(config_set_t *config)
{
    writes++;
    if (!canSave)
        return 0;
    saved[0] = 0;
    for (struct config_value_t *it = config->head; it; it = it->next) {
        strcat(saved, it->key);
        strcat(saved, "=");
        strcat(saved, it->val);
        strcat(saved, "\n");
    }
    config->modified = 0;
    strcpy(persisted[fileIndex(config->filename)], saved);
    return 1;
}
int configRead(config_set_t *config)
{
    char *copy = strdup(persisted[fileIndex(config->filename)]);
    for (char *line = strtok(copy, "\n"); line; line = strtok(NULL, "\n")) {
        char *equals = strchr(line, '=');
        assert(equals);
        *equals++ = 0;
        configSetInt(config, line, atoi(equals));
    }
    free(copy);
    config->modified = 0;
    return 1;
}
void configFree(config_set_t *config)
{
    while (config->head)
        configRemoveKey(config, config->head->key);
    free(config->filename);
    if (config != &registry)
        free(config);
}
static void clearRegistry(void)
{
    while (registry.head) {
        char key[32];
        strcpy(key, registry.head->key);
        configRemoveKey(&registry, key);
    }
}

// Build the real frontend navigation functions into this test translation unit.
typedef struct
{
    int displayedItems;
} items_list_t;
static items_list_t items = {2};
static struct
{
    void *extended;
} element = {&items};
// The navigation source uses gTheme->itemsList->extended.
static struct
{
    typeof(element) *itemsList;
} themePointer = {&element};
#define gTheme (&themePointer)
static menu_list_t selected;
static menu_list_t *selected_item = &selected;
#define SFX_CURSOR 0
static void sfxPlay(int sound)
{
    (void)sound;
}
#include "navigation.inc"

typedef struct item_list
{
    int mode;
    const char *(*itemGetStartup)(struct item_list *, int);
    int enabled;
    char *(*itemGetPrefix)(struct item_list *);
} item_list_t;
#define APP_MODE                   7
#define SFX_CONFIRM                1
#define _STR_ERROR_SAVING_SETTINGS 1
#define _STR_SAVING_SETTINGS       2
#define IO_CUSTOM_SIMPLEACTION     1
#define _l(x)                      "saving"
static int actionStatus, DisableCron, RemainSecs, errors, itemConfigId;
static menu_list_t *menu = &selected;
static void setErrorMessage(int message)
{
    (void)message;
    errors++;
}
static void guiGameHandleDeferedIO(int *pending, void *ui, int type, void *data)
{
    assert(ui == NULL); // Normal screen rendering, no progress overlay.
    (void)type;
    // Reproduce menuRenderElements -> _menuRequestConfig while the save is queued.
    actionStatus = 0;
    assert(*pending == 1);
    ((void (*)(void))data)();
    assert(*pending == 0);
}
#include "favorite_action.inc"
static const char *startup(item_list_t *list, int id)
{
    (void)list;
    (void)id;
    return "SLUS_209.40";
}
static void testImmediateFavorite(void)
{
    submenu_list_t game = {0};
    item_list_t support = {.mode = 0, .itemGetStartup = startup};
    menu_item_t device = {0};
    device.userdata = &support;
    device.submenu = device.current = &game;
    selected.item = &device;
    menuToggleFavorite();
    assert(game.item.favorite && favoritesContains(0, "SLUS_209.40"));
    assert(!strcmp(saved, "SLUS_209.40=1\n"));
    canSave = 0;
    menuToggleFavorite();
    assert(game.item.favorite && errors == 1);
    canSave = 1;
    menuToggleFavorite();
    assert(!game.item.favorite && !saved[0]);
}

static void testFilterPage(void)
{
    submenu_list_t games[7] = {0};
    menu_item_t device = {0};
    for (int i = 0; i < 7; i++) {
        games[i].prev = i ? &games[i - 1] : NULL;
        games[i].next = i < 6 ? &games[i + 1] : NULL;
    }
    games[1].item.favorite = games[5].item.favorite = 1;
    device.submenu = &games[0];
    device.current = device.pagestart = &games[5];
    device.favoritesOnly = 1;
    menuNormalizePage(&device, 4);
    assert(device.current == &games[5] && device.pagestart == &games[1]);
    device.favoritesOnly = 0;
    menuNormalizePage(&device, 4);
    assert(device.current == &games[5] && device.pagestart == &games[3]);
    menuNormalizePage(&device, 4); // Marking must not move an already valid page.
    assert(device.pagestart == &games[3]);
    device.favoritesOnly = 1;
    games[5].item.favorite = 0;
    menuNormalizePage(&device, 4);
    assert(device.current == &games[1] && device.pagestart == &games[1]);
    games[1].item.favorite = 0;
    menuNormalizePage(&device, 4);
    assert(!device.current && !device.pagestart);
}

static void testDeviceFavorites(void)
{
    favoritesLoad(0, "mass0:");
    favoritesLoad(1, "mass1:");
    favoritesLoad(6, "pfs0:OPL/");
    assert(favoritesSet(0, "SLUS_209.40", 1));
    assert(!favoritesContains(1, "SLUS_209.40"));
    assert(!favoritesContains(6, "SLUS_209.40"));
    assert(favoritesSet(6, "SLUS_206.85", 1));
    assert(!favoritesContains(0, "SLUS_206.85"));
    favoritesLoad(0, "mass0:"); // Reload after reboot/refresh.
    assert(favoritesContains(0, "SLUS_209.40"));
    favoritesLoad(0, "mass2:"); // A different USB device took the same mode slot.
    assert(!favoritesContains(0, "SLUS_209.40"));
    favoritesLoad(0, "mass0:");
    assert(favoritesContains(0, "SLUS_209.40"));
    canSave = 0;
    assert(!favoritesSet(0, "SLUS_209.40", 0));
    assert(favoritesContains(0, "SLUS_209.40"));
    assert(favoritesContains(6, "SLUS_206.85"));
    canSave = 1;
    // The same USB library is mounted under a different device number.
    strcpy(persisted[fileIndex("mass4:conf_favorites.cfg")],
           persisted[fileIndex("mass0:conf_favorites.cfg")]);
    favoritesLoad(1, "mass4:");
    assert(favoritesContains(1, "SLUS_209.40"));
    favoritesLoad(2, "smb0:library");
    int foundSMB = 0;
    for (int i = 0; i < 8; i++)
        foundSMB |= !strcmp(paths[i], "smb0:library/conf_favorites.cfg");
    assert(foundSMB);
    favoritesLoad(3, NULL);
    assert(!favoritesSet(3, "SLUS_209.40", 1));
    favoritesEnd();
}

static void testViewPositions(void)
{
    submenu_list_t games[60] = {0};
    menu_item_t device = {0}, secondDevice = {0};
    for (int i = 0; i < 60; i++) {
        games[i].item.id = i;
        games[i].item.favorite = i % 2 == 0;
        games[i].prev = i ? &games[i - 1] : NULL;
        games[i].next = i < 59 ? &games[i + 1] : NULL;
    }
    device.submenu = secondDevice.submenu = &games[0];
    device.current = &games[2];
    device.pagestart = &games[0];
    menuToggleFavoritesView(&device, 12);
    assert(device.favoritesOnly && device.current == &games[0]);
    device.current = &games[42];
    device.pagestart = &games[24];
    menuToggleFavoritesView(&device, 12);
    assert(!device.favoritesOnly && device.current == &games[2] && device.pagestart == &games[0]);
    device.current = &games[8];
    menuToggleFavoritesView(&device, 12);
    assert(device.current == &games[42] && device.pagestart == &games[24]);
    menuToggleFavoritesView(&device, 12);
    assert(device.current == &games[8] && device.pagestart == &games[0]);
    device.current = device.pagestart = &games[59];
    menuToggleFavoritesView(&device, 12);
    menuToggleFavoritesView(&device, 12);
    assert(device.current == &games[59] && device.pagestart == &games[59]);

    secondDevice.current = &games[5];
    secondDevice.pagestart = &games[0];
    menuToggleFavoritesView(&secondDevice, 12);
    menuToggleFavoritesView(&secondDevice, 12);
    assert(secondDevice.current == &games[5]);
    assert(device.current == &games[59]);

    games[42].item.favorite = 0; // The remembered favorite was removed.
    menuToggleFavoritesView(&device, 12);
    assert(device.current == &games[0] && device.pagestart == &games[0]);
    menuToggleFavoritesView(&device, 12);
    for (int i = 0; i < 60; i++)
        games[i].item.favorite = 0;
    menuToggleFavoritesView(&device, 12);
    assert(!device.current && !device.pagestart);
    menuToggleFavoritesView(&device, 12);
    assert(device.current == &games[59] && device.pagestart == &games[59]);
    menuResetViewPositions(&device);
    assert(!device.viewPositions[0].valid && !device.viewPositions[1].valid);
}

static char *prefix(item_list_t *list)
{
    return list->mode == 0 ? "mass0:" : "pfs0:OPL/";
}

static void testStorageSetting(void)
{
    submenu_list_t usbGame = {0}, hddGame = {0};
    item_list_t usb = {.mode = 0, .itemGetStartup = startup, .enabled = 1, .itemGetPrefix = prefix};
    item_list_t hdd = {.mode = 6, .itemGetStartup = startup, .enabled = 1, .itemGetPrefix = prefix};
    menu_item_t usbMenu = {.userdata = &usb, .submenu = &usbGame, .current = &usbGame};
    menu_item_t hddMenu = {.userdata = &hdd, .submenu = &hddGame, .current = &hddGame};
    menu_list_t other = {.item = &hddMenu};
    selected.item = &usbMenu;
    selected.next = &other;
    menuReloadFavorites();
    assert(usbGame.item.favorite && !hddGame.item.favorite);
    gFavoritesStorage = FAVORITES_STORAGE_MC;
    menuReloadFavorites();
    assert(!usbGame.item.favorite && !hddGame.item.favorite);
    menuToggleFavorite();
    assert(usbGame.item.favorite && hddGame.item.favorite);
    assert(strstr(persisted[fileIndex("mc?:OPL/conf_favorites.cfg")], "SLUS_209.40=1"));
    int previousErrors = errors;
    canSave = 0;
    menuToggleFavorite();
    assert(usbGame.item.favorite && hddGame.item.favorite && errors == previousErrors + 1);
    canSave = 1;
    gFavoritesStorage = FAVORITES_STORAGE_DEVICE;
    menuReloadFavorites();
    assert(usbGame.item.favorite && !hddGame.item.favorite);
    gFavoritesStorage = FAVORITES_STORAGE_MC;
    menuReloadFavorites();
    assert(usbGame.item.favorite && hddGame.item.favorite);
    favoritesEnd(); // Reload the master file after a reboot.
    menuReloadFavorites();
    assert(usbGame.item.favorite && hddGame.item.favorite);
    assert(!favoritesSet(7, "SLUS_209.40", 1)); // Apps do not use favorites.
    favoritesEnd();
    selected.next = NULL;
    gFavoritesStorage = FAVORITES_STORAGE_DEVICE;
}

int main(void)
{
    favoritesLoad(0, "mass0:");
    assert(!favoritesContains(0, NULL));
    assert(!favoritesSet(0, "", 1));
    assert(!favoritesSet(0, "#bad", 1));
    assert(!favoritesSet(0, "bad=id", 1));
    assert(!favoritesSet(0, "bad\nname", 1));
    assert(!favoritesSet(0, "01234567890123456789012345678901", 1));
    assert(writes == 0);
    assert(favoritesSet(0, "SLUS_209.40", 1));
    assert(favoritesContains(0, "SLUS_209.40"));
    assert(!strcmp(saved, "SLUS_209.40=1\n"));
    assert(favoritesSet(0, "SLUS_209.40", 1));
    assert(writes == 1);
    clearRegistry(); // Simulate reloading the saved config after a reboot.
    char key[32];
    int value;
    assert(sscanf(saved, "%31[^=]=%d", key, &value) == 2);
    configSetInt(&registry, key, value);
    assert(favoritesContains(0, "SLUS_209.40"));
    canSave = 0;
    assert(!favoritesSet(0, "SLUS_209.40", 0));
    assert(favoritesContains(0, "SLUS_209.40"));
    assert(!favoritesSet(0, "SLUS_206.85", 1));
    assert(!favoritesContains(0, "SLUS_206.85"));
    canSave = 1;
    assert(favoritesSet(0, "SLUS_209.40", 0));
    assert(!favoritesContains(0, "SLUS_209.40"));
    assert(!saved[0]);

    submenu_list_t games[7] = {0};
    menu_item_t menu = {0};
    selected.item = &menu;
    for (int i = 0; i < 7; i++) {
        games[i].item.id = i;
        games[i].prev = i ? &games[i - 1] : NULL;
        games[i].next = i < 6 ? &games[i + 1] : NULL;
    }
    games[1].item.favorite = games[4].item.favorite = games[6].item.favorite = 1;
    menu.submenu = &games[0];
    menu.current = &games[3];
    menuNormalizeSelection(&menu);
    assert(menu.current == &games[3]);
    menu.favoritesOnly = 1;
    menuNormalizeSelection(&menu);
    assert(menu.current == &games[1] && menu.pagestart == &games[1]);
    assert(menuNextVisible(&menu, &games[1]) == &games[4]);
    assert(menuPrevVisible(&menu, &games[6]) == &games[4]);
    menuNextV();
    assert(menu.current == &games[4]);
    menuNextV();
    assert(menu.current == &games[6] && menu.pagestart == &games[6]);
    menuNextV();
    assert(menu.current == &games[1]); // wrap
    menuPrevV();
    assert(menu.current == &games[6]); // reverse wrap
    menuFirstPage();
    assert(menu.current == &games[1]);
    menuNextPage();
    assert(menu.current == &games[6]);
    menuPrevPage();
    assert(menu.current == &games[1]);
    menuLastPage();
    assert(menu.current == &games[6] && menu.pagestart == &games[4]);
    games[6].item.favorite = 0;
    menuNormalizeSelection(&menu);
    assert(menu.current == &games[1]);
    games[1].item.favorite = games[4].item.favorite = 0;
    menuNormalizeSelection(&menu);
    assert(!menu.current && !menu.pagestart);
    menuNextV();
    menuPrevV();
    menuNextPage();
    menuPrevPage();
    menuFirstPage();
    menuLastPage();
    assert(!menu.current); // Every navigation button must be safe on an empty filter.
    menu.favoritesOnly = 0;
    menuNormalizeSelection(&menu);
    assert(menu.current == &games[0]);
    for (int i = 0; i < 7; i++)
        assert(games[i].item.id == i); // IDs and full list remain intact.
    menu.submenu = menu.current = menu.pagestart = NULL;
    menu.favoritesOnly = 1;
    menuNormalizeSelection(&menu);
    assert(!menu.current);
    clearRegistry();
    testImmediateFavorite();
    testFilterPage();
    testViewPositions();
    testDeviceFavorites();
    testStorageSetting();
    puts("PASS: device and master favorites, storage switching, shared markers, reloads, failed saves and paging");
    return 0;
}
