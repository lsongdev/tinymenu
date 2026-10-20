#include "core/tm_ui.h"
#include "pages/tm_grid.h"
#include "pages/tm_tile.h"

#include <stddef.h>

// Every menu page starts like tm_menu, and the tile and grid pages go on like
// tm_icon_menu; keep them in step.
#include "pages/tm_list.h"
#include "popups/tm_list_popup.h"
#include "pages/tm_chart.h"
#define SAME_MENU_FIELD(f)                                                                         \
    _Static_assert(offsetof(tm_list_page, f) == offsetof(tm_menu, f) &&                           \
                       offsetof(tm_chart_page, f) == offsetof(tm_menu, f) &&                       \
                       offsetof(tm_list_popup, f) == offsetof(tm_menu, f) &&                       \
                       offsetof(tm_icon_menu, f) == offsetof(tm_menu, f),                          \
                   "menu pages must start like tm_menu: " #f)
SAME_MENU_FIELD(page);
SAME_MENU_FIELD(item_num);
SAME_MENU_FIELD(option_array);
SAME_MENU_FIELD(select_item);
SAME_MENU_FIELD(icon_w);
SAME_MENU_FIELD(icon_h);
#define SAME_FIELD(f)                                                                              \
    _Static_assert(offsetof(tm_grid_page, f) == offsetof(tm_icon_menu, f) &&                      \
                       offsetof(tm_tile_page, f) == offsetof(tm_icon_menu, f),                     \
                   "tm_tile_page and tm_grid_page must start like tm_icon_menu: " #f)
SAME_FIELD(page);
SAME_FIELD(item_num);
SAME_FIELD(option_array);
SAME_FIELD(select_item);
SAME_FIELD(icon_w);
SAME_FIELD(icon_h);
SAME_FIELD(name_mode);
SAME_FIELD(name_ss);

#define GRID_BOX(g) tm_icon_item_w((g)->icon_w)      // 每一项的选中框宽度
#define GRID_GAP 2                                  // 项与项之间
#define GRID_BAR_W DEFAULT_LIST_BAR_W               // 右侧滚动条

static int16_t pitch(const tm_grid_page* g) {
    return tm_icon_item_h(g->icon_h, g->name_mode != TM_ICON_NAME_NONE) + GRID_GAP; // 行距
}
static int16_t area_h(const tm_grid_page* g) { return g->page.ui->fb.h; }
static int16_t area_w(const tm_ui* ui) { return ui->fb.w - GRID_BAR_W - 1; }
// As many columns as asked for, but no more than fit without the boxes touching.
uint8_t tm_grid_page_cols(const tm_grid_page* g) {
    const uint8_t fit = (uint8_t)MAX(area_w(g->page.ui) / (GRID_BOX(g) + GRID_GAP), 1);
    return MIN(g->cols ? g->cols : TM_GRID_COLS_DEFAULT, fit);
}
static int16_t cell_w(const tm_grid_page* g) { return area_w(g->page.ui) / tm_grid_page_cols(g); }
static int16_t rows(const tm_grid_page* g) {
    return (g->item_num + tm_grid_page_cols(g) - 1) / tm_grid_page_cols(g);
}
static int16_t content_h(const tm_grid_page* g) { return rows(g) * pitch(g); }

// Left edge of the first column's box: the boxes are centred on the whole screen,
// not on the room left of the scroll bar, unless that would put them under the bar.
static int16_t grid_left(const tm_grid_page* g) {
    const int16_t span = (tm_grid_page_cols(g) - 1) * cell_w(g) + GRID_BOX(g);
    return MAX(MIN((g->page.ui->fb.w - span) / 2, area_w(g->page.ui) - span), 0);
}

// Left edge of the selection box of item i, and its top before scrolling.
static int16_t box_x(const tm_grid_page* g, uint8_t i) {
    return grid_left(g) + (int16_t)(i % tm_grid_page_cols(g)) * cell_w(g);
}
static int16_t box_y(const tm_grid_page* g, uint8_t i) {
    return (int16_t)(i / tm_grid_page_cols(g)) * pitch(g) + 1;
}

// Scrolls just enough to show the selected row whole.
static void follow_selection(tm_grid_page* g) {
    const int16_t view = area_h(g);
    const int16_t top = box_y(g, g->select_item) - 1;
    int16_t tgt = g->scroll.pos_tgt;
    if (top < tgt)
        tgt = top;
    if (top + pitch(g) > tgt + view)
        tgt = top + pitch(g) - view;
    const int16_t max = MAX(content_h(g) - view, 0);
    g->scroll.pos_tgt = tgt < 0 ? 0 : tgt > max ? max : tgt;
}

static void draw(tm_grid_page* g) {
    tm_ui* ui = g->page.ui;
    const int16_t view = area_h(g), scroll = g->scroll.pos_cur;
    tm_canvas c = {.fb = &ui->fb, .start_x = 0, .start_y = 0, .w = area_w(ui), .h = view};
    for (uint8_t i = 0; i < g->item_num; i++) {
        const int16_t y = box_y(g, i) - scroll;
        if (y + pitch(g) <= 0 || y >= view)
            continue;
        tm_icon_item_draw(&c, box_x(g, i), y, &g->option_array[i], g->icon_w, g->icon_h,
                          tm_icon_menu_shows_name((tm_icon_menu*)g, i),
                          g->name_mode == TM_ICON_NAME_ALL ? cell_w(g) : area_w(ui),
                          i == g->select_item ? &g->name_ss : NULL, ui->time);
    }
    // Scroll bar, only when there is more than fits.
    const int16_t total = content_h(g);
    if (total > view) {
        const int16_t x = ui->fb.w - 1 - GRID_BAR_W / 2;
        tm_canvas_draw_dashed_line_v(&ui->w_all, x, 0, view - 1, 0x01, 2);
        const int16_t thumb = MAX(view * view / total, 4);
        const int16_t ty = (int16_t)((int32_t)scroll * (view - thumb) / (total - view));
        tm_canvas_draw_rbox(&ui->w_all, ui->fb.w - GRID_BAR_W, ty, GRID_BAR_W, thumb, 0);
    }
}

static void in_init(tm_page_addr addr) {
    tm_grid_page* g = addr;
    follow_selection(g);
    g->scroll.pos_cur = g->scroll.pos_tgt;
    tm_icon_menu_name_reset((tm_icon_menu*)g);
}

static bool grid_in(tm_page_addr addr) {
    tm_grid_page* g = addr;
    tm_ui* ui = g->page.ui;
    tm_animation(&g->scroll, tm_ui_anim_ms(ui, TM_ANI_LIST), ui->time, &ui->anim_is_finish);
    draw(g);
    return g->scroll.pos_cur == g->scroll.pos_tgt;
}

static void show(tm_page_addr addr) {
    grid_in(addr);
    tm_icon_menu_name_step(addr);
}

bool tm_grid_page_move(tm_grid_page* g, int16_t delta) {
    const int16_t n = g->item_num, at = g->select_item;
    int16_t to = at + delta;
    if (delta == 1 || delta == -1) {
        if (to < 0 || to >= n) {
            if (!g->page.ui->params.loop_param[TM_LOOP_TILE])
                return false;
            to = (to + n) % n;
        }
    } else if (to < 0) {
        return false;
    } else if (to >= n) { // down from a row above a short last row: its last item
        if (at / tm_grid_page_cols(g) == (n - 1) / tm_grid_page_cols(g))
            return false;
        to = n - 1;
    }
    tm_grid_page_select(g, (uint8_t)to);
    return true;
}

void tm_grid_page_select(tm_grid_page* g, uint8_t item) {
    if (item >= g->item_num)
        return;
    if (item != g->select_item)
        tm_icon_menu_name_reset((tm_icon_menu*)g);
    g->select_item = item;
    follow_selection(g);
}

static bool react(tm_page_addr addr) {
    tm_grid_page* g = addr;
    tm_ui* ui = g->page.ui;
    bool ret = false;
    tm_msg msg = tm_msg_que_read(&ui->msg_queue);
    if (g->page.auto_input) {
        switch (msg) {
        case TM_MSG_PREV:
        case TM_MSG_LEFT: tm_grid_page_move(g, -1); break;
        case TM_MSG_NEXT:
        case TM_MSG_RIGHT: tm_grid_page_move(g, 1); break;
        case TM_MSG_UP: tm_grid_page_move(g, -tm_grid_page_cols(g)); break;
        case TM_MSG_DOWN: tm_grid_page_move(g, tm_grid_page_cols(g)); break;
        case TM_MSG_RETURN:
            tm_page_return(addr);
            ret = true;
            break;
        default: break;
        }
        if (msg != TM_MSG_NONE && g->page.cb)
            g->page.cb(&g->page, msg);
    } else if (msg != TM_MSG_NONE && g->page.cb) {
        ret = g->page.cb(&g->page, msg);
        if (ret)
            tm_page_return(addr);
    }
    return ret;
}

static void indicator(tm_page_addr addr) {
    tm_grid_page* g = addr;
    tm_ui* ui = g->page.ui;
    ui->indicator.x.pos_tgt = box_x(g, g->select_item);
    ui->indicator.y.pos_tgt = box_y(g, g->select_item) - g->scroll.pos_tgt;
    ui->indicator.w.pos_tgt = GRID_BOX(g);
    ui->indicator.h.pos_tgt = tm_icon_item_h(g->icon_h, false); // 只框图标，不包括名称
    tm_ui_draw_highlight(ui, 3);
}

static const tm_page_methods methods = {
    .in = grid_in, .in_para_init = in_init, .show = show, .react = react,
    .indicator_ctrl = indicator, .scrollbar_ctrl = tm_func_do_nothing,
};

void tm_grid_page_init(tm_grid_page* g, tm_ui* ui, uint8_t item_num, tm_option* option_array,
                       uint8_t cols, tm_page_cb cb) {
    g->page.page_type = TM_PAGE_GRID;
    tm_page_init(g, ui, cb);
    g->page.methods = &methods;
    tm_icon_menu_set_items((tm_icon_menu*)g, item_num, option_array); // 默认 30x30，显示名称
    g->cols = cols;
    g->scroll = (tm_anim){0};
    if (area_h(g) < pitch(g))
        TM_LOG_W("the grid page needs a screen at least %d px high", pitch(g));
    in_init(g);
}
