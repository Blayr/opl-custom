#include "include/opl.h"
#include "include/menusys.h"
#include <string.h>

int menuItemIsVisible(const menu_item_t *menu, const submenu_list_t *item)
{
    return item && (!menu->favoritesOnly || item->item.favorite);
}

submenu_list_t *menuNextVisible(const menu_item_t *menu, submenu_list_t *item)
{
    if (item) {
        do {
            item = item->next;
        } while (item && !menuItemIsVisible(menu, item));
    }
    return item;
}

submenu_list_t *menuPrevVisible(const menu_item_t *menu, submenu_list_t *item)
{
    if (item) {
        do {
            item = item->prev;
        } while (item && !menuItemIsVisible(menu, item));
    }
    return item;
}

submenu_list_t *menuFirstVisible(const menu_item_t *menu)
{
    submenu_list_t *first = menu->submenu;
    return menuItemIsVisible(menu, first) ? first : menuNextVisible(menu, first);
}

void menuNormalizeSelection(menu_item_t *menu)
{
    if (!menuItemIsVisible(menu, menu->current))
        menu->current = menuFirstVisible(menu);
    if (!menu->current)
        menu->pagestart = NULL;
    else if (!menuItemIsVisible(menu, menu->pagestart))
        menu->pagestart = menuFirstVisible(menu);
}

void menuNormalizePage(menu_item_t *menu, int rows)
{
    menuNormalizeSelection(menu);
    if (!menu->current)
        return;
    if (rows < 1)
        rows = 1;

    submenu_list_t *item = menu->pagestart;
    int remaining = rows;
    while (item && item != menu->current && --remaining)
        item = menuNextVisible(menu, item);
    if (item != menu->current) {
        // Find the page containing the selection in the new visible order.
        menu->pagestart = menuFirstVisible(menu);
        int index = 0;
        for (item = menu->pagestart; item != menu->current; item = menuNextVisible(menu, item)) {
            if (++index == rows) {
                menu->pagestart = menuNextVisible(menu, item);
                index = 0;
            }
        }
    }

    // Fill short final pages from above instead of hiding earlier entries.
    int count = 0;
    for (item = menu->pagestart; item && count < rows; item = menuNextVisible(menu, item))
        count++;
    while (count++ < rows && menuPrevVisible(menu, menu->pagestart))
        menu->pagestart = menuPrevVisible(menu, menu->pagestart);
}

void menuResetViewPositions(menu_item_t *menu)
{
    memset(menu->viewPositions, 0, sizeof(menu->viewPositions));
}

static submenu_list_t *menuFindVisibleId(menu_item_t *menu, int id)
{
    for (submenu_list_t *item = menuFirstVisible(menu); item; item = menuNextVisible(menu, item))
        if (item->item.id == id)
            return item;
    return NULL;
}

void menuToggleFavoritesView(menu_item_t *menu, int rows)
{
    int view = !!menu->favoritesOnly;
    menu->viewPositions[view].valid = 1;
    menu->viewPositions[view].currentId = menu->current ? menu->current->item.id : -1;
    menu->viewPositions[view].pageStartId = menu->pagestart ? menu->pagestart->item.id : -1;

    view = !view;
    menu->favoritesOnly = view;
    menu->current = menu->pagestart = NULL;
    if (menu->viewPositions[view].valid) {
        menu->current = menuFindVisibleId(menu, menu->viewPositions[view].currentId);
        menu->pagestart = menuFindVisibleId(menu, menu->viewPositions[view].pageStartId);
    }
    menuNormalizeSelection(menu);

    // Keep an exact saved page when it still contains the selection, including
    // short last pages. Repair only positions invalidated by changes to the list.
    submenu_list_t *item = menu->pagestart;
    for (int row = 0; item && row < rows; row++, item = menuNextVisible(menu, item))
        if (item == menu->current)
            return;
    menuNormalizePage(menu, rows);
}
