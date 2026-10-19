#include "core/tm_ui.h"

//**********************通用页面处理方法
/**
 * @brief 通用的页面返回上一个页面函数
 *
 * @param page_addr 当前页面地址
 */
void tm_page_return(tm_page_addr page_addr) {
    tm_page* p = (tm_page*)page_addr;
    tm_ui* ui = p->ui;
    // 将UI页面设置为上一级页面；没有上一级(last_page 为 NULL)时留在本页面
    ui->in_page = p->last_page ? p->last_page : page_addr;
    // 从主页面返回
    if (p == ui->home_page) { // 主页面返回这个动画很漂亮
        ui->indicator.x.pos_cur = 0;
        ui->indicator.y.pos_cur = 0;
        ui->indicator.w.pos_cur = ui->fb.w;
        ui->indicator.h.pos_cur = ui->fb.h;
    }
}

/**
 * @brief
 *
 * @param page_addr 最基本的页面成员初始化，并加入到页面队列中
 * @param call_back 页面成员点击回调函数
 * @attention 没有进行页面类型初始化，因为这个由各自初始化函数执行
 */
void tm_page_init(tm_page_addr page_addr, tm_ui* ui, tm_page_cb call_back) {
    tm_page* p = (tm_page*)page_addr;
    p->ui = ui;
    if (ui->home_page == NULL) // 第一个页面初始化
    {
        ui->home_page = page_addr;    // 将其初始化为主页面
        ui->current_page = page_addr; // 同时作为当前页面
        ui->in_page = page_addr;      // in页面也初始化为主页面
    }
    p->auto_input = true;
    // 默认页面会自动处理msg输入,回调函数可以进行user的其他处理,如果使用者有一个只读的页面,或者自己有处理输入消息和页面活动,可以在回调函数中调用接口自己处理
    p->last_page = ui->home_page;
    // 页面初始化时没有上一级页面,只有在页面跳转时才确定页面的上下级关系,默认将主页面(第一个初始化的页面)作为上一级页面
    p->cb = call_back;
    p->draw = NULL;
}

void tm_page_set_auto_input(tm_page* page, bool open) {
    if (tm_page_is_inited(page)) {
        page->auto_input = open;
    } else
        TM_LOG_E("this page is not inited!!,it can't disable/enable auto dealwth msg");
}

bool tm_page_is_menu(const void* page) {
    switch (tm_page_type_of(page)) {
    case TM_PAGE_TILE:
    case TM_PAGE_LIST:
    case TM_PAGE_GRID:
    case TM_PAGE_CHART:
    case TM_PAGE_LIST_POPUP: return true;
    default: return false;
    }
}

tm_option* tm_menu_selected(const void* page) {
    if (page == NULL || !tm_page_is_menu(page))
        return NULL;
    const tm_menu* menu = (const tm_menu*)page;
    return menu->item_num ? &menu->option_array[menu->select_item] : NULL;
}

void tm_page_set_draw(tm_page_addr page, tm_page_draw_cb draw) {
    ((tm_page*)page)->draw = draw;
    ((tm_page*)page)->ui->is_motionless = false;
}

static void draw_extra(tm_page* p) {
    if (p->draw) {
        tm_fb_set_color(&p->ui->fb, 1);
        p->draw(p, &p->ui->w_all);
        tm_fb_set_color(&p->ui->fb, 1);
    }
}

void tm_page_show(tm_page_addr page) {
    tm_page* p = page;
    p->methods->show(p);
    draw_extra(p);
}

bool tm_page_in(tm_page_addr page) {
    tm_page* p = page;
    const bool done = p->methods->in(p);
    draw_extra(p);
    return done;
}
