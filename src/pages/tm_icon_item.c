#include "core/tm_ui.h"
#include "pages/tm_icon_item.h"

#define NAME_FONT DEFAULT_LIST_TEXT_FONT
#define NAME_S 1 // 图标与名称之间

int16_t tm_icon_item_w(uint8_t icon_w) {
    return DEFAULT_TILE_ICON_IND_L + icon_w + DEFAULT_TILE_ICON_IND_R;
}

int16_t tm_icon_item_h(uint8_t icon_h, bool name) {
    return DEFAULT_TILE_ICON_IND_U + icon_h + DEFAULT_TILE_ICON_IND_D +
           (name ? NAME_S + GET_FNOT_H(NAME_FONT) : 0);
}

#define MARK_FONT DEFAULT_TILE_B_TITLE_FNOT // 没有图标时，图标位置上的标记字符

void tm_icon_item_draw(tm_canvas* c, int16_t x, int16_t y, const tm_option* option, uint8_t icon_w,
                       uint8_t icon_h, bool show_name, int16_t name_w, tm_slide_str* slide,
                       uint16_t elapsed_ms) {
    const int16_t ix = x + DEFAULT_TILE_ICON_IND_L, iy = y + DEFAULT_TILE_ICON_IND_U;
    if (option->icon) {
        tm_canvas_draw_bmp(c, ix, iy, icon_w, icon_h, option->icon, 1);
    } else { // 没有图标：在图标的位置居中显示标记
        tm_canvas_draw_char(c, ix + (icon_w - MARK_FONT.Width) / 2, iy + (icon_h - MARK_FONT.Height) / 2,
                            MARK_FONT, tm_option_mark(option));
    }
    if (!show_name)
        return;
    const char* name = option->text;
    const int16_t name_y = y + tm_icon_item_h(icon_h, false) + NAME_S; // 在图标的选中框下面
    if (name_y < 0 || name_y + GET_FNOT_H(NAME_FONT) > c->h)
        return; // 名称只在完整可见时绘制
    // The name's room: name_w centred under the icon, so neighbouring names never
    // share a pixel. The name moves with its icon; the canvas only cuts it.
    const int16_t box_w = tm_icon_item_w(icon_w);
    const int16_t room = name_w, centre = c->start_x + x + box_w / 2;
    const int16_t room_l = centre - room / 2;
    const int16_t left = MAX(room_l, c->start_x), right = MIN(room_l + room, c->start_x + c->w);
    if (right <= left)
        return;
    const int16_t text_w = (int16_t)tm_text_width(name, NAME_FONT);
    tm_canvas n = {.fb = c->fb, .start_x = left, .start_y = c->start_y + name_y, .w = right - left,
                   .h = GET_FNOT_H(NAME_FONT)};
    // 图标完整可见时，名称只能用画布里的部分；滑出边缘的图标，名称跟着一起滑出去
    const bool whole = x >= 0 && x + box_w <= c->w;
    if (text_w <= (whole ? n.w : room)) { // 放得下：居中
        int16_t tx = centre - text_w / 2;
        if (whole)
            tx = MAX(MIN(tx, right - text_w), left);
        tm_canvas_draw_str(&n, tx - left, 0, NAME_FONT, name);
    } else if (slide && whole) { // 选中项放不下：和列表一样滚动
        slide->canvas = n;
        slide->str = name;
        tm_slide_str_draw(slide, 0, NAME_FONT, elapsed_ms);
    } else { // 其他项放不下：从开头截断
        tm_canvas_draw_str(&n, room_l - left, 0, NAME_FONT, name);
    }
}

void tm_icon_menu_name_reset(tm_icon_menu* m) {
    const tm_ui* ui = m->page.ui;
    m->name_ss.slide_mode = ui->params.slide_mode;
    m->name_ss.str = NULL; // 还没画过：滚动前先画一次，确定它的画布
    m->name_item = m->select_item;
    tm_slide_str_reset(&m->name_ss);
}

tm_slide_str* tm_icon_menu_name_slide(tm_icon_menu* m, uint8_t i) {
    if (i != m->select_item)
        return NULL;
    if (m->name_item != m->select_item) // 不管怎样换的选中项，每次聚焦都从头滚动
        tm_icon_menu_name_reset(m);
    return &m->name_ss;
}

void tm_icon_menu_name_step(tm_icon_menu* m) {
    tm_ui* ui = m->page.ui;
    if (m->name_ss.str && ui->anim_is_finish && !m->name_ss.slide_is_finish)
        m->name_ss.slide_enable = true; // 其他动画结束，且这一轮没滚完就开始滚动
    ui->slide_is_finish = ui->slide_is_finish && !m->name_ss.slide_enable;
}

void tm_icon_menu_set_items(tm_icon_menu* m, uint8_t item_num, tm_option* option_array) {
    m->item_num = item_num;
    m->option_array = option_array;
    m->select_item = 0;
    m->icon_w = TILE_ICON_W;
    m->icon_h = TILE_ICON_H;
    m->name_mode = TM_ICON_NAME_ALL;
    tm_icon_menu_name_reset(m);
    for (uint8_t i = 0; i < item_num; i++) {
        option_array[i].order = i; // 选项序号标号
        if (!option_array[i].text) {
            option_array[i].text = TM_POPUP_TEXT_DEFAULT; // 绘制前不检查 NULL，这里给个默认文本
            TM_LOG_E("The text of %d th icon item is not set", i);
        }
    }
}
