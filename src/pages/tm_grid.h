#ifndef TM_PAGES_TM_GRID_H_
#define TM_PAGES_TM_GRID_H_

#include "core/tm_page.h"
#include "pages/tm_icon_item.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TM_GRID_COLS_DEFAULT 3

/* A page of icon items (tm_icon_item) in a matrix, `cols` to a row: each icon
 * with its name under it, the selected one only, or icons only (name_mode; set
 * after init). It scrolls up and down to keep the selection in view. Left/right and
 * prev/next move one item (wrapping with ui.params.loop_param[TM_LOOP_TILE]);
 * up/down move one row. Click goes to the callback, return goes back. */
/* The first fields are tm_icon_menu's, in the same order as in tm_tile_page. */
typedef struct tm_grid_page {
    tm_page page;
    uint8_t item_num;
    tm_option* option_array; /* item_num entries; each draws its icon */
    uint8_t select_item;
    uint8_t icon_w, icon_h;      /* size of the options' icons (default 30x30) */
    tm_icon_name_mode name_mode; /* names under the icons (default all) */
    tm_slide_str name_ss;        /* the selected name, scrolling when it does not fit */
    uint8_t cols;   /* columns asked for (0: TM_GRID_COLS_DEFAULT); see tm_grid_page_cols */
    tm_anim scroll; /* pixels the grid is scrolled down by */
} tm_grid_page;

/* cols 0 selects TM_GRID_COLS_DEFAULT; fewer are used if they do not fit
 * (a column takes the icon's width + 8 px: three 30 px icons need 118 px). */
void tm_grid_page_init(tm_grid_page* grid, tm_ui* ui, uint8_t item_num, tm_option* option_array,
                       uint8_t cols, tm_page_cb cb);
/* Columns in use: cols, or fewer when they do not fit at the current icon size. */
uint8_t tm_grid_page_cols(const tm_grid_page* grid);
/* Moves the selection by delta items (cols for a row); false if it cannot move. */
bool tm_grid_page_move(tm_grid_page* grid, int16_t delta);
void tm_grid_page_select(tm_grid_page* grid, uint8_t item);

#ifdef __cplusplus
}
#endif
#endif
