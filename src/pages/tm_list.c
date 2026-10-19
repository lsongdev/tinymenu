#include "core/tm_ui.h"
#include "pages/tm_list.h"
#include "pages/tm_tile.h"
#include "pages/tm_grid.h"

//===================================================List页面====================================================
// 列表参数,所有列表使用同一套参数
#define LIST_TEXT_FONT DEFAULT_LIST_TEXT_FONT // 列表每行文字字体
#define LIST_TEXT_U_S DEFAULT_LIST_TEXT_U_S   // 列表每行文字的上边距
#define LIST_TEXT_D_S DEFAULT_LIST_TEXT_D_S   // 列表每行文字的上边距
#define LIST_TEXT_LINE_H (GET_FNOT_H(LIST_TEXT_FONT) + LIST_TEXT_U_S + LIST_TEXT_D_S) // 文字列表的行高
#define LIST_ICON_PAD 2 // 图标上下、与文字之间的边距
// 行内文字和选框的上边距：行高比文字行高时居中
#define ROW_TEXT_Y(s) (LIST_TEXT_U_S + ((s)->line_h - LIST_TEXT_LINE_H) / 2)
#define LIST_TEXT_L_S DEFAULT_LIST_TEXT_L_S   // 列表每行文字的左边距
#define LIST_IND_VAL_S DEFAULT_LIST_IND_VAL_S // 指示器和val文本的间距
#define LIST_BAR_W DEFAULT_LIST_BAR_W // 列表进度条宽度，需要是奇数，因为正中间有1像素宽度的线
#define LIST_TEXT_R_S                                                                              \
    (DEFAULT_LIST_TEXT_R_S + LIST_BAR_W) // 列表每行结尾的数值的右边距（包括进度条宽度 LIST_BAR_W）
#define LIST_TEXT_MAX_LEN(ui)                                                                          \
    (DEFAULT_LIST_TEXT_LEN_PER_MAX * (ui)->fb.w / 100) // 列表文字的最大宽度
#define LIST_VAL_MAX_LEN(ui)                                                                           \
    ((ui)->fb.w - LIST_TEXT_MAX_LEN(ui) - LIST_TEXT_R_S - LIST_TEXT_L_S * 2 -                   \
     LIST_IND_VAL_S) // 这里左间距*2是为了给val和text之间留空间
#define LIST_VAL_BUFF_SIZE DEFAULT_LIST_VAL_BUFF_SIZE // 列表值文本缓冲区的大小
#define LIST_IND_BOX_R DEFAULT_LIST_IND_BOX_R         // 列表指示器盒子倒角
// 列表行尾选择选项参数
#define CHECK_BOX_U_S LIST_TEXT_U_S // 选择框在每行的上边距(与文字保持同高)
#define CHECK_BOX_R_S LIST_TEXT_R_S // 选择框的右边距（包括进度条宽度 LIST_BAR_W）(与值文字对齐)
#define CHECK_BOX_F_H GET_FNOT_H(LIST_TEXT_FONT) // 选择框外框高度(与文字高度保持一致)
#define CHECK_BOX_F_W CHECK_BOX_F_H              // 选择框外框宽度(方形，与高度保持一致)
#define CHECK_BOX_D_S DEFAULT_LIST_CHECK_BOX_D_S // 选择框里面的点距离外框的边距
#define CHECK_BOX_D_W                                                                              \
    CHECK_BOX_F_W - 2 * (CHECK_BOX_D_S + 1) // 选择框里面的点宽度(这里加的常量1是外框的线条宽)
#define CHECK_BOX_D_H                                                                              \
    CHECK_BOX_F_H - 2 * (CHECK_BOX_D_S + 1)      // 选择框里面的点高度(这里加的常量1是外框的线条宽)
#define CHECK_BOX_R DEFAULT_LIST_CHECK_BOX_R     // 列表确认选择框倒角
#define SWITCH_W (CHECK_BOX_F_H * 2 - 2)         // 开关样式的宽度(高度与选择框相同)
#define CHECK_BOX_D_R DEFAULT_LIST_CHECK_BOX_D_R // 列表确认选择框内点的倒角
#define LIST_TXT_SLIDESTR_MODE DEFAULT_LIST_TXT_SLIDESTR_MODE // 磁贴标题文本的滚动模式
#define LIST_VAL_SLIDESTR_MODE DEFAULT_LIST_VAL_SLIDESTR_MODE // 磁贴标题文本的滚动模式
#define LIST_MACRO_ASSERT(ui)                                                                          \
    ((CHECK_BOX_R * 2 > CHECK_BOX_F_W) || (CHECK_BOX_D_R * 2 > CHECK_BOX_D_W) ||                   \
     (LIST_VAL_MAX_LEN(ui) < CHECK_BOX_F_W))

//--------List相关函数
// What comes before a row's text: its icon in an icon list (when it has one), else its mark ("@ ").
static bool row_has_icon(const tm_list_page* self, const tm_option* item) {
    return self->icon_w && item->icon;
}
static int16_t row_lead_w(const tm_list_page* self, const tm_option* item) {
    return row_has_icon(self, item) ? self->icon_w + 2 * LIST_ICON_PAD : LIST_TEXT_FONT.Width * 2;
}
// Width of a row's mark or icon and its text.
static int16_t list_option_text_width(const tm_list_page* self, const tm_option* item) {
    return (int16_t)(row_lead_w(self, item) + tm_text_width(item->text, LIST_TEXT_FONT));
}

static void tm_list_auto_canvas_draw_line_tail_val_txt(tm_list_page* self, 
    tm_option* item, tm_canvas* p_canvas_txt, tm_canvas* p_canvas_val, const char* val_buff,
    uint8_t select_order) {
    tm_ui* ui = self->page.ui; // 这个函数纯粹因为用的次数多，分离出来的，没有什么特殊作用
    int16_t val_str_len = (int16_t)tm_text_width(val_buff, LIST_TEXT_FONT);
    int16_t txt_str_len = list_option_text_width(self, item);
    if (txt_str_len + LIST_TEXT_L_S * 2 + LIST_TEXT_R_S + val_str_len + LIST_IND_VAL_S <
        (ui)->fb.w) {
        // 两个字符串的宽度之和不超过屏幕宽度，则按长短调整
        p_canvas_txt->w = txt_str_len;
        p_canvas_val->start_x =
            (ui)->fb.w - val_str_len - LIST_TEXT_R_S; // 放得下的时候值靠右侧放
        p_canvas_val->w = val_str_len;
    } else if (val_str_len < LIST_VAL_MAX_LEN(ui)) {
        // 如果长度之和超过屏幕宽度，但值放得下，则值优先靠右侧放
        p_canvas_val->start_x =
            (ui)->fb.w - val_str_len - LIST_TEXT_R_S; // 放得下的时候值靠右侧放
        p_canvas_txt->w = p_canvas_val->start_x - 2 * LIST_TEXT_L_S - LIST_IND_VAL_S;
        p_canvas_val->w = val_str_len;
    }
    if (item->order == select_order) {
        self->vars.opt_val_ss.canvas = *p_canvas_val;
        self->vars.opt_val_ss.str = val_buff;
        tm_slide_str_draw(&(self->vars.opt_val_ss), 0, LIST_TEXT_FONT, ui->time);
        if (ui->anim_is_finish && !(self->vars.opt_val_ss.slide_is_finish)) {
            self->vars.opt_val_ss.slide_enable =
                true; // anim结束，且单次滚动没有完成就使能滚动
        }
    } else
        tm_canvas_draw_str(p_canvas_val, 0, 0, LIST_TEXT_FONT, val_buff);
}

// A switch at (x, y): a capsule with its knob on the left when off; filled, with the
// knob cut out on the right, when on.
static void draw_switch(tm_ui* ui, int16_t x, int16_t y, bool on) {
    const int16_t h = CHECK_BOX_F_H, knob = h - 4;
    const int16_t r = tm_ui_radius(ui, h / 2), kr = tm_ui_radius(ui, knob / 2);
    const int16_t knob_x = on ? x + SWITCH_W - 2 - knob : x + 2;
    if (on) {
        tm_canvas_draw_rbox(&ui->w_all, x, y, SWITCH_W, h, r);
        tm_fb_set_color(&ui->fb, 0);
        tm_canvas_draw_rbox(&ui->w_all, knob_x, y + 2, knob, knob, kr);
        tm_fb_set_color(&ui->fb, 1);
    } else {
        tm_canvas_draw_rbox_empty(&ui->w_all, x, y, SWITCH_W, h, r);
        tm_canvas_draw_rbox(&ui->w_all, knob_x, y + 2, knob, knob, kr);
    }
}

static void tm_list_draw_text_check_box(tm_list_page* self, int16_t start_y, tm_option* item, uint8_t select_order) {
    tm_ui* ui = self->page.ui;
    tm_canvas canvas_txt = {.fb = &ui->fb,
                         .start_x = LIST_TEXT_L_S,
                         .start_y = (int16_t)(start_y + ROW_TEXT_Y(self)),
                         .w = LIST_TEXT_MAX_LEN(ui),
                         .h = (int16_t)(LIST_TEXT_FONT.Height)};
    tm_canvas canvas_val = {.fb = &ui->fb,
                         .start_x = LIST_TEXT_L_S * 2 + LIST_TEXT_MAX_LEN(ui) + LIST_IND_VAL_S,
                         .start_y = (int16_t)(start_y + ROW_TEXT_Y(self)),
                         .w = LIST_VAL_MAX_LEN(ui),
                         .h = (int16_t)(LIST_TEXT_FONT.Height)};
    // 绘制表尾
    char val_buff[LIST_VAL_BUFF_SIZE];
    switch (item->kind) {
    case TM_OPTION_INT:
        tm_format_int(val_buff, sizeof(val_buff), item->val);
        tm_list_auto_canvas_draw_line_tail_val_txt(self, item, &canvas_txt, &canvas_val, val_buff,
                                                select_order);
        break;
    case TM_OPTION_FIXED:
        tm_format_fixed(val_buff, sizeof(val_buff), item->val, item->decimals);
        tm_list_auto_canvas_draw_line_tail_val_txt(self, item, &canvas_txt, &canvas_val, val_buff,
                                                select_order);
        break;
    case TM_OPTION_SWITCH:
        canvas_txt.w = MIN(list_option_text_width(self, item),
                           (ui)->fb.w - CHECK_BOX_R_S - SWITCH_W - LIST_TEXT_L_S * 2 - LIST_IND_VAL_S);
        draw_switch(ui, (ui)->fb.w - CHECK_BOX_R_S - SWITCH_W, start_y + ROW_TEXT_Y(self),
                    item->val != 0);
        break;
    case TM_OPTION_CHECK:
        canvas_txt.w = MIN(list_option_text_width(self, item),
                           (ui)->fb.w - CHECK_BOX_R_S - CHECK_BOX_F_W - LIST_TEXT_L_S * 2 -
                               LIST_IND_VAL_S); // 除确认框外的宽度，其他都算在文字宽度中
        tm_canvas_draw_rbox_empty(
            &(ui->w_all), (ui)->fb.w - CHECK_BOX_R_S - CHECK_BOX_F_W,
            start_y + ROW_TEXT_Y(self), CHECK_BOX_F_W, CHECK_BOX_F_H, tm_ui_radius(ui, CHECK_BOX_R));
        if (item->val != 0) // 非0即是选中，画框内的点(这里加的常量1是外框的线条宽)
            tm_canvas_draw_rbox(&(ui->w_all),
                                  (ui)->fb.w - CHECK_BOX_R_S - CHECK_BOX_F_W +
                                      CHECK_BOX_D_S + 1,
                                  start_y + ROW_TEXT_Y(self) + CHECK_BOX_D_S + 1, CHECK_BOX_D_W,
                                  CHECK_BOX_D_H, tm_ui_radius(ui, CHECK_BOX_D_R));
        break;
    case TM_OPTION_TEXT:
        if (NULL != item->content) {
            tm_list_auto_canvas_draw_line_tail_val_txt(self, item, &canvas_txt, &canvas_val, item->content,
                                                    select_order);
        }
        break;
    default:
        canvas_txt.w = MIN(list_option_text_width(self, item),
                           (ui)->fb.w - LIST_TEXT_R_S - LIST_TEXT_L_S * 2);
        break;
    }
    // 绘制标记(图标列表里是图标)和文字
    if (row_has_icon(self, item)) {
        tm_canvas_draw_bmp(&(ui->w_all), canvas_txt.start_x, start_y + (self->line_h - self->icon_h) / 2,
                           self->icon_w, self->icon_h, item->icon, 1);
    } else {
        char mark[3] = {tm_option_mark(item), ' ', 0};
        tm_canvas_draw_str(&(canvas_txt), 0, 0, LIST_TEXT_FONT, mark);
    }
    if (item->order == select_order) {
        self->vars.indicator_w_temp =
            LIST_TEXT_L_S * 2 + canvas_txt.w; // 计算出每一项的宽度后，顺便给inductor的宽度赋值
        canvas_txt.start_x += row_lead_w(self, item);
        canvas_txt.w -= row_lead_w(self, item); // 去掉前面标识符(或图标)的长度
        self->vars.opt_text_ss.canvas = canvas_txt;
        self->vars.opt_text_ss.str = (char*)item->text;
        tm_slide_str_draw(&(self->vars.opt_text_ss), 0, LIST_TEXT_FONT, ui->time);
        if (ui->anim_is_finish && !(self->vars.opt_text_ss.slide_is_finish)) {
            self->vars.opt_text_ss.slide_enable =
                true; // anim结束，且单次滚动没有完成就使能滚动
        }
    } else
        tm_canvas_draw_str(&(canvas_txt), row_lead_w(self, item), 0, LIST_TEXT_FONT, item->text);
}

void tm_list_page_in_init(tm_page_addr page_addr) {
    tm_list_page* self = (tm_list_page*)page_addr;
    tm_ui* ui = self->page.ui;
    UNUSED_PARAMETER(page_addr);
    self->vars.optInt.pos_cur = 0;
    self->vars.optInt.pos_tgt = self->line_h;
    ui->scrollBar.display = true;
    // step其实可以移到init中，因为这个值在整个状态机中基本是是不变的
    self->vars.opt_text_ss.slide_mode = ui->params.slidestrmode_param[TM_SLIDE_STEP_LIST_TEXT];
    self->vars.opt_val_ss.slide_mode = ui->params.slidestrmode_param[TM_SLIDE_STEP_LIST_TEXT];
    self->vars.opt_text_ss.step = ui->params.slidestrstep_param[TM_SLIDE_STEP_LIST_TEXT];
    self->vars.opt_val_ss.step = ui->params.slidestrstep_param[TM_SLIDE_STEP_LIST_VALUE];
    tm_slide_str_reset(&(self->vars.opt_text_ss));
    tm_slide_str_reset(&(self->vars.opt_val_ss));
    self->vars.cascade_ms = 0;
    self->vars.cascading = ui->params.style.transition == TM_TRANSITION_CASCADE;
}

// Distance left after t ms of an animation over `dist` with tm_animation's easing
// (each 10 ms frame covers 10/duration of what remains), in 1/256 px.
static int32_t eased_left(int32_t dist, int32_t t, int32_t duration) {
    int32_t left = dist << 8;
    for (; t >= 10 && left >= 256; t -= 10)
        left -= left * 10 / duration;
    return left - left * MAX(t, 0) / duration;
}

// TM_TRANSITION_CASCADE: the rows on screen slide in one after another, top to bottom,
// from the right going deeper and from the left coming back, eased like every other
// animation. Ends in the same state as the unfold.
static bool list_cascade_in(tm_list_page* self) {
    tm_ui* ui = self->page.ui;
    const int16_t line_h = self->line_h;
    const int16_t top = self->ind_y_tgt - line_h * self->select_item; // 第一行最终的y
    const int32_t dur = tm_ui_anim_ms(ui, TM_ANI_SLIDE); // 与其他动画相同的缓动和时长参数
    const int32_t lag = dur / 2;                          // 相邻两行开始滑入的间隔
    const int8_t from = ui->slide_dir < 0 ? -1 : 1;
    bool done = true;
    int32_t rank = 0;
    self->vars.cascade_ms = (uint16_t)MIN((uint32_t)self->vars.cascade_ms + ui->time, 60000u);
    self->vars.cascade_sel_x = 0;
    for (uint8_t i = 0; i < self->item_num; i++) {
        const int16_t y = top + line_h * i;
        if (y <= -line_h || y >= ui->fb.h)
            continue; // 只有屏幕上的行参与
        const int32_t t = (int32_t)self->vars.cascade_ms - lag * rank++;
        int16_t x = 0;
        if (dur > 0) {
            const int32_t left = t < 0 ? (int32_t)ui->fb.w << 8 : eased_left(ui->fb.w, t, dur);
            if (left >= 256) { // 还差至少 1 像素
                x = (int16_t)(from * (left >> 8));
                done = false;
            }
        }
        if (i == self->select_item)
            self->vars.cascade_sel_x = x;
        ui->fb.ox = x;
        tm_list_draw_text_check_box(self, y, &(self->option_array[i]), self->select_item);
    }
    ui->fb.ox = 0;
    if (done) {
        self->vars.cascading = false;
        self->vars.optInt.pos_cur = self->vars.optInt.pos_tgt = top; // 与展开结束时一致
    } else {
        ui->anim_is_finish = false;
    }
    return done;
}

bool tm_list_page_in(tm_page_addr page_addr) {
    tm_list_page* self = (tm_list_page*)page_addr;
    tm_ui* ui = self->page.ui;
    if (self->vars.cascading)
        return list_cascade_in(self);
    bool ret = false; // 默认动画没有完成
    int16_t item_y = 0; // 用于临时存放列表每一项的y坐标
    const uint8_t list_line_h = self->line_h; // 因为行高是用宏计算的，这里用变量保存不用每次都计算
    tm_animation(&self->vars.optInt, tm_ui_anim_ms(ui, TM_ANI_LIST), ui->time,
                     &(ui->anim_is_finish)); // 文字y坐标
    for (uint8_t i = 0; i < (self->item_num); i++) {
        if (ui->params.ufd_param[TM_UNFOLD_LIST]) // 从头展开
            item_y = i * (self->vars.optInt.pos_cur) - list_line_h * (self->select_item) +
                     self->ind_y_tgt;
        else // 选中项展开
            item_y = (i - (self->select_item)) * (self->vars.optInt.pos_cur) + self->ind_y_tgt;
        tm_list_draw_text_check_box(self, item_y, &(self->option_array[i]), self->select_item);
    }
    if (self->vars.optInt.pos_cur == self->vars.optInt.pos_tgt) { // 动画抵达目标位置
        ret = true;
        self->vars.optInt.pos_cur = self->vars.optInt.pos_tgt =
            -list_line_h * (self->select_item) + self->ind_y_tgt;
        // 过度动画完成后optInt不再表示行间距了，而是表示第一个选项顶部的位置与屏幕顶部的距离(而且是个负数)
    }
    return ret;
}

void tm_list_page_show(tm_page_addr page_addr) {
    tm_list_page* self = (tm_list_page*)page_addr;
    tm_ui* ui = self->page.ui;
    int16_t item_y = 0; // 用于临时存放列表每一项的y坐标
    const uint8_t list_line_h = self->line_h; // 因为行高是用宏计算的，这里用变量保存不用每次都计算
    // 计算动画过渡值
    tm_animation(&self->vars.optInt, tm_ui_anim_ms(ui, TM_ANI_LIST), ui->time,
                     &(ui->anim_is_finish)); // 文字y坐标
    for (uint8_t i = 0; i < (self->item_num); i++) {
        item_y = list_line_h * i + (self->vars.optInt.pos_cur);
        tm_list_draw_text_check_box(self, item_y, &(self->option_array[i]), self->select_item);
    }
    ui->slide_is_finish =
        !(self->vars.opt_text_ss.slide_enable) &&
        !(self->vars.opt_val_ss.slide_enable); // 记录这个页面中所有slide动画的结果&&
}

void tm_list_page_indicator_ctrl(tm_page_addr page_addr) {
    tm_list_page* self = (tm_list_page*)page_addr;
    tm_ui* ui = self->page.ui;
    // indicator
    ui->indicator.x.pos_tgt = 0;
    ui->indicator.y.pos_tgt = self->ind_y_tgt;
    ui->indicator.w.pos_tgt = ui->params.style.highlight_width == TM_HIGHLIGHT_FULL_ROW
                                  ? (ui)->fb.w - LIST_BAR_W - 1 // 整行，与滚动条留 1 像素
                                  : self->vars.indicator_w_temp;
    ui->indicator.h.pos_tgt = self->line_h;
    if (self->vars.cascading) { // 选中框跟着选中行一起滑入
        ui->indicator.x.pos_cur = ui->indicator.x.pos_tgt + self->vars.cascade_sel_x;
        ui->indicator.y.pos_cur = ui->indicator.y.pos_tgt;
        ui->indicator.w.pos_cur = ui->indicator.w.pos_tgt;
        ui->indicator.h.pos_cur = ui->indicator.h.pos_tgt;
    }
    tm_ui_draw_highlight(ui, LIST_IND_BOX_R);
}

void tm_list_page_scroll_bar_ctrl(tm_page_addr page_addr) {
    tm_list_page* self = (tm_list_page*)page_addr;
    tm_ui* ui = self->page.ui;
    // scrollBar
    ui->scrollBar.y.pos_tgt = self->item_num > 1 // 只有一项时停在顶部
        ? (int16_t)(self->select_item * (ui)->fb.h / (self->item_num - 1)) + 1 : 1;
    tm_canvas_draw_line_h(&(ui->w_all), (ui)->fb.w - LIST_BAR_W, (ui)->fb.w,
                            0);
    tm_canvas_draw_line_h(&(ui->w_all), (ui)->fb.w - LIST_BAR_W, (ui)->fb.w,
                            (ui)->fb.h - 1);
    tm_canvas_draw_line_v(&(ui->w_all), (ui)->fb.w - ((LIST_BAR_W / 2) + 1), 0,
                            (ui)->fb.h);
    tm_canvas_draw_rbox(&(ui->w_all), (ui)->fb.w - LIST_BAR_W, 0, LIST_BAR_W,
                          ui->scrollBar.y.pos_cur, 0);
}

static void list_prev(tm_list_page* lp);
static void list_next(tm_list_page* lp);

bool tm_list_page_react(tm_page_addr page_addr) {
    tm_list_page* self = (tm_list_page*)page_addr;
    tm_ui* ui = self->page.ui;
    bool ret = false;
    tm_msg msg = tm_msg_que_read(&(ui->msg_queue));
    if (self->page.auto_input) {
        switch (msg) {
        case TM_MSG_PREV:
        case TM_MSG_UP:
        case TM_MSG_LEFT: // 竖排：上下为主，左右当作上一个/下一个
            list_prev(self);
            break;
        case TM_MSG_NEXT:
        case TM_MSG_DOWN:
        case TM_MSG_RIGHT:
            list_next(self);
            break;
        case TM_MSG_CLICK: // 如果没有使用自动处理msg的话,二值框的值和是否是单选项肯定也是用户自己处理嘞
            if (self->option_array[self->select_item].kind == TM_OPTION_SWITCH) // 开关：翻转值
                self->option_array[self->select_item].val =
                    !self->option_array[self->select_item].val;
            // 单选页面：只有选中的选框为真
            if (self->mode == TM_LIST_RADIO) {
                for (uint8_t i = 0; i < self->item_num; i++) {
                    const tm_option_kind kind = self->option_array[i].kind;
                    if (kind == TM_OPTION_SWITCH || kind == TM_OPTION_CHECK) {
                        if (i != self->select_item)
                            self->option_array[i].val = false;
                        else
                            self->option_array[i].val = true; // 确保单选
                    }
                }
            }
            break;
        case TM_MSG_RETURN:
            tm_page_return(page_addr);
            ret = true; // 通知UI切换状态
            break;
        default:
            break;
        }
        if (TM_MSG_NONE != msg && NULL != self->page.cb) // 任何输入的有效信息都会调用回调函数
            self->page.cb(&(self->page), msg);
    } else {
        if (TM_MSG_NONE != msg && NULL != self->page.cb) { // 任何输入的有效信息都会调用回调函数
            ret = self->page.cb(&(self->page), msg); // 使用者在回调函数返回true,表示退出这个页面
            if (ret == true)
                tm_page_return(page_addr);
        }
    }
    return ret;
}

static const tm_page_methods tm_list_page_methods = {
    .in = tm_list_page_in,
    .in_para_init = tm_list_page_in_init,
    .show = tm_list_page_show,
    .react = tm_list_page_react,
    .indicator_ctrl = tm_list_page_indicator_ctrl,
    .scrollbar_ctrl = tm_list_page_scroll_bar_ctrl,
};

void tm_list_page_init(
    tm_list_page* lp,
    tm_ui* ui,             // 列表页面对象
    uint8_t item_num,         // 选项个数，
    tm_option* option_array,     // 整个页面的选项数组(数组大小需与item_num一致)
    tm_list_mode mode, // 页面设置
    tm_page_cb call_back    // 回调函数，参数为确认选中项index（1-256）0表示未确认哪个选项)
) {
    if (LIST_MACRO_ASSERT(ui))
        TM_LOG_W("The chamfer of the check box is too large, exceeding its width.");
    lp->page.page_type = TM_PAGE_LIST;
    lp->page.methods = &tm_list_page_methods;
    tm_page_init((tm_page_addr)lp, ui, call_back);
    memset(&lp->vars, 0, sizeof(lp->vars));
    lp->vars.opt_text_ss.slide_mode = (tm_slide_mode)LIST_TXT_SLIDESTR_MODE;
    lp->vars.opt_val_ss.slide_mode = (tm_slide_mode)LIST_VAL_SLIDESTR_MODE;
    lp->vars.opt_text_ss.canvas.fb = &ui->fb;
    lp->vars.opt_val_ss.canvas.fb = &ui->fb;
    lp->mode = mode;
    lp->item_num = item_num;
    lp->select_item = 0;
    lp->option_array = option_array;
    lp->icon_w = lp->icon_h = 0; // 文字列表
    lp->line_h = LIST_TEXT_LINE_H;
    lp->line_n = (ui)->fb.h / lp->line_h;
    for (uint8_t i = 0; i < lp->item_num; i++) {
        lp->option_array[i].order = i; // 选项序号标号
        if (NULL == lp->option_array[i].text) {
            // text不能没有设置，因为在darwstr之前没有检查text文本是否为NULL，这里LOG_E提示
            lp->option_array[i].text = TM_POPUP_TEXT_DEFAULT; // 使用默认文本提示noset
            TM_LOG_E("The text of %d th item in ListPgae NO set!!!", i);
        }
    }
}
void tm_list_page_set_icon_size(tm_list_page* lp, uint8_t icon_w, uint8_t icon_h) {
    lp->icon_w = icon_h ? icon_w : 0;
    lp->icon_h = icon_w ? icon_h : 0;
    lp->line_h = (uint8_t)MAX(LIST_TEXT_LINE_H, lp->icon_h ? lp->icon_h + 2 * LIST_ICON_PAD : 0);
    lp->line_n = MAX(lp->page.ui->fb.h / lp->line_h, 1);
    lp->vars.optInt.pos_cur = 0;
    tm_list_page_select(lp, 0); // 行高变了，从第一行重新开始
}
static void list_prev(tm_list_page* lp) {
    tm_ui* ui = lp->page.ui; // 重置动画参数
    tm_slide_str_reset(&(lp->vars.opt_text_ss));
    tm_slide_str_reset(&(lp->vars.opt_val_ss));
    const uint8_t list_line_h = lp->line_h; // 因为行高是用宏计算的，这里用变量保存不用每次都计算
    if (lp->select_item == 0) {                       // 选中第一个的话
        if (ui->params.loop_param[TM_LOOP_LIST]) { // 同时loop参数开的话，从顶部滑动到底部
            lp->select_item = lp->item_num - 1;       // 选中最后一个
            if (lp->item_num > lp->line_n) {          // 数目超出一页的最大数目
                lp->ind_y_tgt = (ui)->fb.h - list_line_h; // 更改box到最底
                lp->vars.optInt.pos_tgt =
                    (ui)->fb.h - (lp->item_num) * list_line_h; // 更改文字到最底
            } else                                                     // 没有超出数目则是到最后一个
                lp->ind_y_tgt = (lp->item_num - 1) * list_line_h;
        }
    } else {               // 没有选中第一个
        lp->select_item--; // 选中减1
        if ((lp->select_item - 1) <
            -((lp->vars.optInt.pos_tgt) / list_line_h)) { // 光标盒子到页面顶了
            if (!((ui)->fb.h % list_line_h))              // 上面剩余完整的行
                lp->vars.optInt.pos_tgt += list_line_h;   // 文字上移
            else { // 上面的行不完整(list_line_h该项用于页面和行高非整除时)
                if (lp->ind_y_tgt ==
                    (ui)->fb.h -
                        list_line_h *
                            lp->line_n) { // 文字往下走一行，且光标盒子置于0处，把上面的非整行去掉
                    lp->vars.optInt.pos_tgt +=
                        (lp->line_n + 1) * list_line_h - (ui)->fb.h;
                    lp->ind_y_tgt = 0;
                } else if (lp->ind_y_tgt == list_line_h)
                    lp->ind_y_tgt = 0; // 上面整行直接移动光标盒子
                else
                    lp->vars.optInt.pos_tgt += list_line_h; // 顶页整行，文字直接往下走
            }
        } else                            // 光标盒子没到页面顶
            lp->ind_y_tgt -= list_line_h; // 直接光标盒子往上走即可。
    }
}
static void list_next(tm_list_page* lp) {
    tm_ui* ui = lp->page.ui; // 重置动画参数
    tm_slide_str_reset(&(lp->vars.opt_text_ss));
    tm_slide_str_reset(&(lp->vars.opt_val_ss));
    const uint8_t list_line_h = lp->line_h; // 因为行高是用宏计算的，这里用变量保存不用每次都计算
    if (lp->select_item == (lp->item_num) - 1) {      // 到最后一个选项了
        if (ui->params.loop_param[TM_LOOP_LIST]) { // loop开关开,全部回到顶部
            lp->vars.optInt.pos_tgt = 0;
            lp->select_item = 0;
            lp->ind_y_tgt = 0;
        }
    } else { // 不是最后一个选项
        lp->select_item++;
        if ((lp->select_item + 1) >
            ((lp->line_n) - (lp->vars.optInt.pos_tgt) / list_line_h)) { // 光标到页面底
            if (!((ui)->fb.h % list_line_h))                            // 上面剩余完整的行
                lp->vars.optInt.pos_tgt -= list_line_h;                 // 文字下移
            else {                                                              // 非整行的情况
                if (lp->ind_y_tgt == list_line_h * (lp->line_n - 1)) {
                    lp->vars.optInt.pos_tgt -=
                        (lp->line_n + 1) * list_line_h - (ui)->fb.h;
                    // 取出要向下移动的距离，对y_tgt来说就是向上移动的距离
                    lp->ind_y_tgt = (ui)->fb.h - list_line_h;
                } else if (lp->ind_y_tgt ==
                           (ui)->fb.h - list_line_h * 2) // 这种情况什么时候会出现呢嗯?
                    lp->ind_y_tgt = (ui)->fb.h - list_line_h;
                else
                    lp->vars.optInt.pos_tgt -= list_line_h;
            }
        } else
            lp->ind_y_tgt += list_line_h;
    }
}

bool tm_list_page_move(tm_list_page* lp, int16_t delta) {
    const uint8_t from = lp->select_item;
    for (; delta < 0; delta++) list_prev(lp);
    for (; delta > 0; delta--) list_next(lp);
    return lp->select_item != from;
}

void tm_list_page_select(tm_list_page* lp, uint8_t index) {
    if (index >= lp->item_num)
        return;
    lp->select_item = 0; // 从第一行走过去，滚动位置跟着算好
    lp->ind_y_tgt = 0;
    lp->vars.optInt.pos_tgt = 0;
    for (uint8_t i = 0; i < index; i++) list_next(lp);
}
