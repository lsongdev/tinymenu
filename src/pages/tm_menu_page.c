#include "core/tm_ui.h"
#include "pages/tm_menu_page.h"

bool tm_menu_move(tm_page_addr page, int16_t delta) {
    switch (tm_page_type_of(page)) {
    case TM_PAGE_LIST: return tm_list_page_move(page, delta);
    case TM_PAGE_TILE: return tm_tile_page_move(page, delta);
    case TM_PAGE_GRID: return tm_grid_page_move(page, delta);
    case TM_PAGE_CHART: return tm_chart_page_move(page, delta);
    case TM_PAGE_LIST_POPUP: return tm_list_popup_move(page, delta);
    default: return false;
    }
}

void tm_menu_select(tm_page_addr page, uint8_t index) {
    switch (tm_page_type_of(page)) {
    case TM_PAGE_LIST: tm_list_page_select(page, index); break;
    case TM_PAGE_TILE: tm_tile_page_select(page, index); break;
    case TM_PAGE_GRID: tm_grid_page_select(page, index); break;
    case TM_PAGE_CHART: tm_chart_page_select(page, index); break;
    case TM_PAGE_LIST_POPUP: tm_list_popup_select(page, index); break;
    default: break;
    }
}

static void init_layout(tm_menu_page* p, tm_ui* ui, uint8_t item_num, tm_option* options,
                        tm_menu_layout layout, tm_page_cb cb) {
    switch (layout) {
    case TM_MENU_GRID: tm_grid_page_init(&p->grid, ui, item_num, options, 0, cb); break;
    case TM_MENU_ICON_LIST:
    case TM_MENU_LIST:
        tm_list_page_init(&p->list, ui, item_num, options, TM_LIST_NORMAL, cb);
        if (layout == TM_MENU_ICON_LIST)
            tm_list_page_set_icon_size(&p->list, TILE_ICON_W, TILE_ICON_H);
        break;
    case TM_MENU_TILE:
    default: tm_tile_page_init(&p->tile, ui, item_num, options, cb); break;
    }
}

void tm_menu_page_init(tm_menu_page* p, tm_ui* ui, uint8_t item_num, tm_option* option_array,
                       tm_menu_layout layout, tm_page_cb cb) {
    memset(p, 0, sizeof(*p));
    init_layout(p, ui, item_num, option_array, layout, cb);
}

tm_menu_layout tm_menu_page_layout(const tm_menu_page* p) {
    switch (p->menu.page.page_type) {
    case TM_PAGE_GRID: return TM_MENU_GRID;
    case TM_PAGE_LIST: return p->list.icon_w ? TM_MENU_ICON_LIST : TM_MENU_LIST;
    default: return TM_MENU_TILE;
    }
}

void tm_menu_page_set_layout(tm_menu_page* p, tm_menu_layout layout) {
    tm_ui* ui = p->menu.page.ui;
    const tm_page saved = p->menu.page; // 上级页面、回调和消息处理方式
    const uint8_t item_num = p->menu.item_num, select = p->menu.select_item;
    tm_option* const options = p->menu.option_array;
    init_layout(p, ui, item_num, options, layout, saved.cb);
    p->menu.page.last_page = saved.last_page;
    p->menu.page.auto_input = saved.auto_input;
    p->menu.page.draw = saved.draw;
    tm_menu_select(p, select); // 回到原来选中的那一项，滚动位置也跟着算好
    if (ui->current_page == (tm_page_addr)p) { // 正在显示：以新的排列重新进入
        p->menu.page.methods->in_para_init(p);
        ui->in_page = p;
        ui->state = TM_STATE_IN;
        tm_ui_invalidate(ui);
    }
}
