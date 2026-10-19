#ifndef TM_PAGES_TM_MENU_PAGE_H_
#define TM_PAGES_TM_MENU_PAGE_H_

#include "pages/tm_grid.h"
#include "pages/tm_list.h"
#include "pages/tm_tile.h"
#include "pages/tm_chart.h"
#include "popups/tm_list_popup.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Moving the selection of any menu page (list, tile, grid, chart, list popup): by delta
 * items, as the page itself would on next/prev, or straight to an item. */
bool tm_menu_move(tm_page_addr page, int16_t delta);
void tm_menu_select(tm_page_addr page, uint8_t index);

typedef enum tm_menu_layout {
    TM_MENU_TILE = 0,  /* icons and names in a row, the selection in the middle */
    TM_MENU_GRID,      /* icons and names in rows of three */
    TM_MENU_ICON_LIST, /* a list with each item's icon in place of its mark */
    TM_MENU_LIST,      /* a plain list: marks and text */
} tm_menu_layout;

/* One menu that can be shown in any layout and switched between them while
 * the program runs. It is a single page at a single address, so the pages it
 * opens still return to it, and the home page stays the home page. */
typedef union tm_menu_page {
    tm_menu menu; /* the fields every layout shares: items, selection, icon size */
    tm_list_page list;
    tm_tile_page tile;
    tm_grid_page grid;
} tm_menu_page;

void tm_menu_page_init(tm_menu_page* page, tm_ui* ui, uint8_t item_num, tm_option* option_array,
                       tm_menu_layout layout, tm_page_cb cb);
/* Lays the same items out another way, keeping the selection, the callback and
 * the page it returns to. On screen, it enters again in the new layout. */
void tm_menu_page_set_layout(tm_menu_page* page, tm_menu_layout layout);
tm_menu_layout tm_menu_page_layout(const tm_menu_page* page);

#ifdef __cplusplus
}
#endif
#endif
