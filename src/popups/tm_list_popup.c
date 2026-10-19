#include "core/tm_ui.h"
#include "popups/tm_list_popup.h"

//--------------------------------------------------ListWin页面------------------------------------------------------------------
// 宏定义
#define LIST_WIN_FONT DEFAULT_LIST_WIN_FONT                          // 弹窗字体
#define LIST_WIN_W DEFAULT_LIST_WIN_W                                // 弹窗宽度
#define LIST_WIN_R DEFAULT_LIST_WIN_R                                // 弹窗倒角
#define LIST_WIN_BOX_R DEFAULT_LIST_WIN_BOX_R                        // 列表选择框倒角
#define LIST_WIN_L_S DEFAULT_LIST_WIN_L_S                            // 列表左边距
#define LIST_WIN_R_S (DEFAULT_LIST_WIN_R_S + DEFAULT_LIST_WIN_BAR_W) // 列表右边距不含进度条
#define LIST_WIN_TEXT_U_S DEFAULT_LIST_WIN_TEXT_U_S                  // 列表每行文字的上边距
#define LIST_WIN_TEXT_D_S DEFAULT_LIST_WIN_TEXT_D_S                  // 列表每行文字的下边距
#define LIST_WIN_TEXT_LINE_H                                                                       \
    (GET_FNOT_H(LIST_WIN_FONT) + LIST_WIN_TEXT_U_S + LIST_WIN_TEXT_D_S) // 文字行高
#define LIST_WIN_ICON_PAD 2 // 图标上下、与文字之间的边距
#define LIST_WIN_LINE_H (lw->vars.line_h)
// 行内文字的上边距：行高比文字行高时居中
#define ROW_TEXT_Y(lw) (LIST_WIN_TEXT_U_S + ((lw)->vars.line_h - LIST_WIN_TEXT_LINE_H) / 2)
#define LIST_WIN_BAR_W DEFAULT_LIST_WIN_BAR_W                           // 默认进度条的宽度

//----------ListWinPage相关函数
static const char* list_popup_str(const tm_list_popup* lw, uint8_t i) {
    return lw->option_array[i].text;
}

// What comes before a row's text: its icon (with an icon size), or a mark it sets itself.
static int16_t row_lead_w(const tm_list_popup* lw, const tm_option* item) {
    if (lw->icon_w && item->icon)
        return lw->icon_w + 2 * LIST_WIN_ICON_PAD;
    return item->mark ? LIST_WIN_FONT.Width * 2 : 0;
}
static void draw_lead(tm_list_popup* lw, const tm_canvas* row, const tm_option* item, int16_t row_y) {
    tm_ui* ui = lw->page.ui;
    if (lw->icon_w && item->icon) {
        tm_canvas_draw_bmp(&ui->w_all, row->start_x, row_y + (lw->vars.line_h - lw->icon_h) / 2,
                           lw->icon_w, lw->icon_h, item->icon, 1);
    } else if (item->mark) {
        char mark[2] = {item->mark, 0};
        tm_canvas_draw_str((tm_canvas*)row, 0, ROW_TEXT_Y(lw), LIST_WIN_FONT, mark);
    }
}

static void list_popup_draw(tm_list_popup* lw) {
    tm_ui* ui = lw->page.ui;
    tm_canvas canvas_temp = {.fb = &ui->fb};
    // 弹窗外框
    tm_canvas_draw_rbox_empty(&(ui->w_all), lw->vars.win_x.pos_cur, -1,
                               LIST_WIN_W + LIST_WIN_R, (ui)->fb.h + 2, tm_ui_radius(ui, LIST_WIN_R));
    canvas_temp.h = LIST_WIN_LINE_H;
    canvas_temp.w = LIST_WIN_W - 2 * LIST_WIN_L_S - LIST_WIN_R_S;
    canvas_temp.start_x = lw->vars.win_x.pos_cur + LIST_WIN_L_S;
    lw->vars.str_ss.canvas = canvas_temp;
    // 绘制选项
    for (uint8_t i = 0; i < lw->item_num; i++) {
        const tm_option* item = &lw->option_array[i];
        const int16_t row_y = LIST_WIN_LINE_H * i + (lw->vars.list_y.pos_cur);
        const int16_t lead = row_lead_w(lw, item);
        canvas_temp.start_y = row_y;
        draw_lead(lw, &canvas_temp, item, row_y);
        tm_canvas text = canvas_temp;
        text.start_x += lead;
        text.w -= lead;
        if (i != lw->select_item) {
            tm_canvas_draw_str(&text, 0, ROW_TEXT_Y(lw), LIST_WIN_FONT, list_popup_str(lw, i));
        } else {
            lw->vars.str_ss.str = list_popup_str(lw, i);
            lw->vars.str_ss.canvas = text;
            tm_slide_str_draw(&(lw->vars.str_ss), ROW_TEXT_Y(lw), LIST_WIN_FONT, ui->time);
        }
    }
}

bool tm_list_popup_in(tm_page_addr page_addr) {
    tm_list_popup* self = (tm_list_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    bool ret = false;
    // 窗口x坐标运动
    tm_animation(&self->vars.win_x, tm_ui_anim_ms(ui, TM_ANI_POPUP), ui->time,
                     &(ui->anim_is_finish));
    // 弹窗填充
    tm_fb_set_color(&ui->fb, 0);
    tm_canvas_draw_rbox(&(ui->w_all), self->vars.win_x.pos_cur, -1,
                          LIST_WIN_W + LIST_WIN_R, (ui)->fb.h + 2, tm_ui_radius(ui, LIST_WIN_R));
    tm_fb_set_color(&ui->fb, 1);
    list_popup_draw(self);
    if (self->vars.win_x.pos_cur == self->vars.win_x.pos_tgt)
        ret = true;
    return ret;
}
void tm_list_popup_in_init(tm_page_addr page_addr) {
    tm_list_popup* self = (tm_list_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    self->bg_opt = tm_menu_selected(self->page.last_page);
    self->vars.win_x.pos_cur = (ui)->fb.w;
    self->vars.win_x.pos_tgt = (ui)->fb.w - LIST_WIN_W;
    self->vars.line_h = (uint8_t)MAX(LIST_WIN_TEXT_LINE_H,
                                     self->icon_h ? self->icon_h + 2 * LIST_WIN_ICON_PAD : 0);
    self->vars.line_n = MAX((ui)->fb.h / self->vars.line_h, 1);
    self->vars.str_ss.slide_mode = ui->params.slidestrmode_param[TM_SLIDE_STEP_POPUP_TEXT];
    self->vars.str_ss.step = ui->params.slidestrstep_param[TM_SLIDE_STEP_POPUP_TEXT];
    tm_slide_str_reset(&(self->vars.str_ss));
    ui->win_bg_blur = ui->params.winbgblur_param[TM_POPUP_BLUR_LIST];
}
void tm_list_popup_show(tm_page_addr page_addr) {
    tm_list_popup* self = (tm_list_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    tm_page* bg = (tm_page*)(((tm_page*)page_addr)->last_page);
    tm_page_show(bg); // 先绘制背景
    tm_fb_set_color(&ui->fb, TM_PEN_BLACK);
    tm_fb_blur(&ui->fb, ui->win_bg_blur); // 背景虚化
    tm_canvas_draw_rbox(&(ui->w_all), self->vars.win_x.pos_cur, -1,
                          LIST_WIN_W + LIST_WIN_R, (ui)->fb.h + 2, tm_ui_radius(ui, LIST_WIN_R));
    tm_fb_set_color(&ui->fb, TM_PEN_WHITE);
    // 选项移动动画
    tm_animation(&self->vars.list_y, tm_ui_anim_ms(ui, TM_ANI_LIST), ui->time,
                     &(ui->anim_is_finish));
    list_popup_draw(self);
    if (ui->anim_is_finish) { // anim结束，且单次滚动没有完成就使能滚动
        if (!self->vars.str_ss.slide_is_finish)
            self->vars.str_ss.slide_enable = true;
    }
    // 记录这个页面中所有slide动画的结果&&(同时处理背景中的slide动画)
    ui->slide_is_finish =
        ui->slide_is_finish && !(self->vars.str_ss.slide_enable);
}
static void popup_prev(tm_list_popup* lw);
static void popup_next(tm_list_popup* lw);

bool tm_list_popup_react(tm_page_addr page_addr) {
    tm_list_popup* self = (tm_list_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    bool ret = false;
    tm_msg msg = tm_msg_que_read(&(ui->msg_queue));
    if (tm_msg_is_direct_msg(msg))
        tm_slide_str_reset(&(self->vars.str_ss));
    if (self->page.auto_input) {
        switch (msg) {
        case TM_MSG_PREV:
        case TM_MSG_UP:
        case TM_MSG_LEFT:
            popup_prev(self);
            break;
        case TM_MSG_NEXT:
        case TM_MSG_RIGHT:
        case TM_MSG_DOWN:
            popup_next(self);
            break;
        case TM_MSG_CLICK:
            if (self->auto_set_bg_opt) { // 如果使能自动赋值bg页面选中项值的话
                tm_option* opt = tm_menu_selected(self->page.last_page);
                if (NULL != opt)
                    opt->content = list_popup_str(self, self->select_item);
            }
            // fall through - a click also closes the popup
        case TM_MSG_RETURN:
            tm_page_return(page_addr);
            self->vars.win_x.pos_tgt = (ui)->fb.w; // 弹窗滑动退出动画
            ret = true;
            break;
        default:
            break;
        }
        if (TM_MSG_NONE != msg && NULL != self->page.cb)
            self->page.cb(&(self->page), msg); // 任何输入的有效信息都会调用回调函数
    } else {
        if (TM_MSG_NONE != msg && NULL != self->page.cb) { // 任何输入的有效信息都会调用回调函数
            ret = self->page.cb(&(self->page), msg);
            if (ret) { // 如果回调函数返回true的话退出该页面
                tm_page_return(page_addr);
                self->vars.win_x.pos_tgt = (ui)->fb.w; // 弹窗滑动退出动画
            }
        }
    }
    return ret;
}
void tm_list_popup_indicator_ctrl(tm_page_addr page_addr) {
    tm_list_popup* self = (tm_list_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    ui->indicator.x.pos_tgt = (ui)->fb.w - LIST_WIN_W + (LIST_WIN_L_S >> 1);
    ui->indicator.y.pos_tgt = self->vars.ind_y_temp;
    const int16_t row_w = LIST_WIN_W - LIST_WIN_L_S - LIST_WIN_R_S;
    ui->indicator.w.pos_tgt =
        ui->params.style.highlight_width == TM_HIGHLIGHT_FULL_ROW
            ? row_w
            : MIN(row_lead_w(self, &self->option_array[self->select_item]) +
                      tm_text_width(list_popup_str(self, self->select_item), LIST_WIN_FONT) + LIST_WIN_L_S,
                  row_w);
    ui->indicator.h.pos_tgt = self->vars.line_h;
    tm_ui_draw_highlight(ui, LIST_WIN_BOX_R);
}
void tm_list_popup_scroll_bar_ctrl(tm_page_addr page_addr) {
    tm_list_popup* self = (tm_list_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    ui->scrollBar.y.pos_tgt = self->item_num > 1
        ? (int16_t)(self->select_item * (ui)->fb.h / (self->item_num - 1)) + 1 : 1;
    tm_canvas_draw_line_h(&(ui->w_all), (ui)->fb.w - LIST_WIN_BAR_W,
                            (ui)->fb.w, 0);
    tm_canvas_draw_line_h(&(ui->w_all), (ui)->fb.w - LIST_WIN_BAR_W,
                            (ui)->fb.w, (ui)->fb.h - 1);
    tm_canvas_draw_line_v(&(ui->w_all), (ui)->fb.w - ((LIST_WIN_BAR_W / 2) + 1), 0,
                            (ui)->fb.h);
    tm_canvas_draw_rbox(&(ui->w_all), (ui)->fb.w - LIST_WIN_BAR_W, 0, LIST_WIN_BAR_W,
                          ui->scrollBar.y.pos_cur, 0);
}

static const tm_page_methods tm_list_popup_methods = {
    .in = tm_list_popup_in,
    .in_para_init = tm_list_popup_in_init,
    .show = tm_list_popup_show,
    .react = tm_list_popup_react,
    .indicator_ctrl = tm_list_popup_indicator_ctrl,
    .scrollbar_ctrl = tm_list_popup_scroll_bar_ctrl,
};

void tm_list_popup_init(tm_list_popup* lw, tm_ui* ui, uint8_t item_num, tm_option* option_array,
                        bool auto_set_bg_opt, tm_page_cb cb) {
    if (NULL == lw || NULL == option_array) {
        TM_LOG_E("lw is NULL or option_array is NULL!");
        return;
    }
    lw->page.page_type = TM_PAGE_LIST_POPUP;
    tm_page_init((tm_page_addr)lw, ui, cb);
    memset(&lw->vars, 0, sizeof(lw->vars));
    lw->vars.str_ss.slide_mode = (tm_slide_mode)TM_SLIDE_ONCE;
    lw->vars.str_ss.canvas.fb = &ui->fb;
    lw->item_num = item_num;
    lw->option_array = option_array;
    lw->select_item = 0;
    lw->icon_w = lw->icon_h = 0;
    lw->auto_set_bg_opt = auto_set_bg_opt;
    lw->page.methods = &tm_list_popup_methods;
    lw->bg_opt = NULL;
    lw->vars.line_h = LIST_WIN_TEXT_LINE_H;
    lw->vars.line_n = MAX(ui->fb.h / lw->vars.line_h, 1);
    for (uint8_t i = 0; i < item_num; i++) {
        option_array[i].order = i;
        if (NULL == option_array[i].text) {
            option_array[i].text = TM_POPUP_TEXT_DEFAULT; // 绘制前不检查 NULL，这里给个默认文本
            TM_LOG_E("The text of %d th option in the list popup is not set", i);
        }
    }
}

static void popup_prev(tm_list_popup* lw) {
    tm_ui* ui = lw->page.ui;
    int16_t list_line_h = LIST_WIN_LINE_H;
    if (lw->select_item == 0) {                          // 选中第一个的话
        if (ui->params.loop_param[TM_LOOP_LIST_POPUP]) {  // 同时loop参数开的话
            lw->select_item = lw->item_num - 1;         // 选中最后一个
            if (lw->item_num > lw->vars.line_n) { // 数目超出一页的最大数目
                lw->vars.ind_y_temp = (ui)->fb.h - list_line_h; // 更改box到最底
                lw->vars.list_y.pos_tgt =
                    (ui)->fb.h - (lw->item_num) * list_line_h; // 更改文字到最底
            } else // 没有超出数目则是到最后一个
                lw->vars.ind_y_temp = (lw->item_num - 1) * list_line_h;
        }
    } else {                 // 没有选中第一个
        lw->select_item--; // 选中减1
        if ((lw->select_item - 1) <
            -((lw->vars.list_y.pos_tgt) / list_line_h)) { // 光标盒子到页面顶了????
            if (!((ui)->fb.h % list_line_h))              // 上面剩余完整的行
                lw->vars.list_y.pos_tgt += list_line_h;   // 文字下移
            else { // 上面的行不完整(list_line_h该项用于页面和行高非整除时)
                if (lw->vars.ind_y_temp ==
                    (ui)->fb.h -
                        list_line_h *
                            lw->vars
                                .line_n) { // 文字往下走一行，且光标盒子置于0处，把上面的非整行去掉
                    lw->vars.list_y.pos_tgt +=
                        (lw->vars.line_n + 1) * list_line_h - (ui)->fb.h;
                    lw->vars.ind_y_temp = 0;
                } else if (lw->vars.ind_y_temp == list_line_h)
                    lw->vars.ind_y_temp = 0; // 上面整行直接移动光标盒子
                else
                    lw->vars.list_y.pos_tgt += list_line_h; // 顶页整行，文字直接往下走
            }
        } else                                          // 光标盒子没到页面顶
            lw->vars.ind_y_temp -= list_line_h; // 直接光标盒子往上走即可。
    }
}

void tm_list_popup_select(tm_list_popup* lw, uint8_t index) {
    tm_ui* ui = lw->page.ui;
    const int16_t list_line_h = LIST_WIN_LINE_H;
    if (index >= lw->item_num)
        return;
    lw->select_item = index;
    if ((index + 1) * list_line_h <= (ui)->fb.h) { // 在第一屏内：列表不动，移动光标
        lw->vars.list_y.pos_tgt = 0;
        lw->vars.ind_y_temp = index * list_line_h;
    } else { // 滚动列表，让选中项停在最底下一行
        lw->vars.list_y.pos_tgt = (ui)->fb.h - (index + 1) * list_line_h;
        lw->vars.ind_y_temp = (ui)->fb.h - list_line_h;
    }
    lw->vars.list_y.pos_cur = lw->vars.list_y.pos_tgt;
    tm_slide_str_reset(&(lw->vars.str_ss));
}
static void popup_next(tm_list_popup* lw) {
    tm_ui* ui = lw->page.ui;
    int16_t list_line_h = LIST_WIN_LINE_H;
    if (lw->select_item == (lw->item_num) - 1) {       // 到最后一个选项了
        if (ui->params.loop_param[TM_LOOP_LIST_POPUP]) { // loop开关开,全部回到顶部
            lw->vars.list_y.pos_tgt = 0;
            lw->select_item = 0;
            lw->vars.ind_y_temp = 0;
        }
    } else { // 不是最后一个选项
        lw->select_item++;
        if ((lw->select_item + 1) >
            ((lw->vars.line_n) -
             (lw->vars.list_y.pos_tgt) / list_line_h)) { // 光标到页面底
            if (!((ui)->fb.h % list_line_h))             // 上面剩余完整的行
                lw->vars.list_y.pos_tgt -= list_line_h;  // 文字上移
            else {                                               // 非整行的情况
                if (lw->vars.ind_y_temp == list_line_h * (lw->vars.line_n - 1)) {
                    lw->vars.list_y.pos_tgt -=
                        (lw->vars.line_n + 1) * list_line_h - (ui)->fb.h;
                    // 取出要向下移动的距离，对y_tgt来说就是向上移动的距离
                    lw->vars.ind_y_temp = (ui)->fb.h - list_line_h;
                } else if (lw->vars.ind_y_temp ==
                           (ui)->fb.h - list_line_h * 2) // 这种情况什么时候会出现呢嗯?
                    lw->vars.ind_y_temp = (ui)->fb.h - list_line_h;
                else
                    lw->vars.list_y.pos_tgt -= list_line_h;
            }
        } else
            lw->vars.ind_y_temp += list_line_h;
    }
}

bool tm_list_popup_move(tm_list_popup* lw, int16_t delta) {
    const uint8_t from = lw->select_item;
    for (; delta < 0; delta++) popup_prev(lw);
    for (; delta > 0; delta--) popup_next(lw);
    if (lw->select_item != from)
        tm_slide_str_reset(&(lw->vars.str_ss));
    return lw->select_item != from;
}
