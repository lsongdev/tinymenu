#ifndef TM_PAGES_TM_CHART_H_
#define TM_PAGES_TM_CHART_H_

#include "core/tm_page.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum tm_chart_type {
    TM_CHART_BAR = 0, /* columns, labels under them when they fit */
    TM_CHART_HBAR,    /* a row per item: label, bar, value; scrolls like a list */
    TM_CHART_LINE,    /* points joined by lines */
} tm_chart_type;

/* A chart of options: each item is a bar or a point, with its text as the
 * label and its val (shown with its decimals) as the value. It is a menu like
 * the others: the first fields are tm_menu's, the selection moves along the
 * items in the style's highlight, a click goes to the callback (a value popup
 * opened from here edits the selected item) and return goes back. Change the
 * values at any time and call tm_chart_page_refresh, e.g. for a spectrum. */
typedef struct tm_chart_page {
    tm_page page;
    uint8_t item_num;
    tm_option* option_array;
    uint8_t select_item;
    uint8_t icon_w, icon_h; /* unused: charts draw no icons */
    tm_chart_type type;
    int32_t min, max; /* the value axis; both 0 (the default): from the values, including 0 */
    struct {
        tm_anim scroll; /* pixels scrolled along the items */
        tm_anim grow;   /* entry: bars grow from the base line, 0..256 */
    } vars;
} tm_chart_page;

void tm_chart_page_init(tm_chart_page* chart, tm_ui* ui, uint8_t item_num, tm_option* option_array,
                        tm_chart_type type, tm_page_cb cb);
/* Redraws after the application changed values (or type, min, max). */
void tm_chart_page_refresh(tm_chart_page* chart);
/* Moves the selection by delta items (wrapping with loop_param[TM_LOOP_LIST]); false if
 * it did not move. */
bool tm_chart_page_move(tm_chart_page* chart, int16_t delta);
void tm_chart_page_select(tm_chart_page* chart, uint8_t index);

#ifdef __cplusplus
}
#endif
#endif
