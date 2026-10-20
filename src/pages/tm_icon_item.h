#ifndef TM_PAGES_TM_ICON_ITEM_H_
#define TM_PAGES_TM_ICON_ITEM_H_

#include "core/tm_page.h"
#include "gfx/tm_text.h"

#ifdef __cplusplus
extern "C" {
#endif

// The default icon size of the tile and grid pages, and a type for icons of that size.
#define TILE_ICON_W DEFAULT_TILE_ICON_W // 图标宽度
#define TILE_ICON_H DEFAULT_TILE_ICON_H // 图标高度
#define ICON_BUFFSIZE (TILE_ICON_W * (TILE_ICON_H / 8 + 1))
typedef const uint8_t tm_icon[ICON_BUFFSIZE]; // 图标：页优先的位图，与帧缓冲同样的排列

/* An icon item: an option's icon (its mark when it has none) with its name
 * under it, or the icon only. The tile and grid pages both draw their items
 * with it; the selection marks the icon. Sizes are of the item's box: the
 * icon (icon_w x icon_h) with the highlight's margins around it, and the
 * name under that when shown. */
int16_t tm_icon_item_w(uint8_t icon_w);
int16_t tm_icon_item_h(uint8_t icon_h, bool name);
/* Draws the item whose box has its top left at (x, y) on canvas c. The name has
 * name_w centred on the box (kept on the canvas): one that fits is centred in it,
 * a longer one is cut, or scrolls with `slide` (the selected item's; NULL for the
 * others) advanced by elapsed_ms, as a list scrolls its selected row. */
void tm_icon_item_draw(tm_canvas* c, int16_t x, int16_t y, const tm_option* option, uint8_t icon_w,
                       uint8_t icon_h, bool name, int16_t name_w, tm_slide_str* slide,
                       uint16_t elapsed_ms);

/* Which icons have their name under them. Room for a name is kept under every
 * icon unless NONE, so the icons stay put as the selection moves. */
typedef enum tm_icon_name_mode {
    TM_ICON_NAME_NONE = 0, /* icons only */
    TM_ICON_NAME_ALL,      /* every icon (the default) */
    TM_ICON_NAME_SELECTED, /* the selected icon only */
} tm_icon_name_mode;

/* What the tile and grid pages share: tm_menu's fields, then these, in the
 * same order in both. */
typedef struct tm_icon_menu {
    tm_page page;
    uint8_t item_num;
    tm_option* option_array; /* item_num entries; each draws its icon */
    uint8_t select_item;
    uint8_t icon_w, icon_h;      /* size of the options' icons (default 30x30) */
    tm_icon_name_mode name_mode; /* which icons have their name under them */
    tm_slide_str name_ss;        /* the selected item's name, when it scrolls */
} tm_icon_menu;

/* Sets the items of a tile or grid page (after tm_page_init), numbers them and
 * gives the page the default icon size and every name. */
void tm_icon_menu_set_items(tm_icon_menu* menu, uint8_t item_num, tm_option* option_array);
/* Starts the selected name over: from its beginning, scrolling once the page is still. */
void tm_icon_menu_name_reset(tm_icon_menu* menu);
/* After drawing the items: lets the selected name scroll once the other
 * animations have finished, and keeps the UI awake while it does. */
void tm_icon_menu_name_step(tm_icon_menu* menu);
/* Whether item i shows its name. */
static inline bool tm_icon_menu_shows_name(const tm_icon_menu* menu, uint8_t i) {
    return menu->name_mode == TM_ICON_NAME_ALL ||
           (menu->name_mode == TM_ICON_NAME_SELECTED && i == menu->select_item);
}

#ifdef __cplusplus
}
#endif
#endif
