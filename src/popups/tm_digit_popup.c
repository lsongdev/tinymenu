#include "core/tm_ui.h"
#include "popups/tm_digit_popup.h"

//---------------------------------------------------------DigitWin页面---------------------------------------------------
// 宏定义
#define DIGIT_WIN_FONT DEFAULT_DIGIT_WIN_FONT                  // 弹窗字体
#define DIGIT_WIN_NUM_FONT DEFAULT_DIGIT_WIN_NUM_FONT          // 数字字体
#define DIGIT_WIN_STR_BUFF_SIZE DEFAULT_VAL_WIN_STR_BUFF_SIZE // 临时buff的大小
#define DIGIT_WIN_FONT_MARGIN DEFAULT_DIGIT_WIN_FONT_MARGIN    // 弹窗内元素到边框的间距
#define DIGIT_WIN_W(ui) MIN(DEFAULT_DIGIT_WIN_W, (ui)->fb.w - 8) // 弹窗宽度(屏幕窄时收窄)
#define DIGIT_WIN_NUM_S DEFAULT_DIGIT_WIN_NUM_S                // 每个数字的间距
#define DIGIT_WIN_V_S DEFAULT_DIGIT_WIN_V_S                    // 页面内元素间纵向的间距
#define DIGIT_WIN_BOX_R DEFAULT_DIGIT_WIN_BOX_R                // 弹窗指示器倒角
#define DIGIT_WIN_BOX_H DEFAULT_DIGIT_WIN_BOX_H                // 弹窗指示器在数字底部时的高度
#define DIGIT_WIN_R DEFAULT_DIGIT_WIN_R
#define DIGIT_WIN_H                                                                                 \
    (2 * DIGIT_WIN_FONT_MARGIN + 2 * GET_FNOT_H(DIGIT_WIN_NUM_FONT) + GET_FNOT_H(DIGIT_WIN_FONT) +    \
     2 * DIGIT_WIN_V_S + DIGIT_WIN_BOX_H) // 弹窗高度
#define DIGIT_WIN_Y(ui) (((ui)->fb.h - DIGIT_WIN_H) >> 1)
#define DIGIT_WIN_X(ui) (((ui)->fb.w - DIGIT_WIN_W(ui)) >> 1)
#define DIGIT_WIN_TXT_Y_OFS DIGIT_WIN_FONT_MARGIN // 文本y坐标相对边框的偏移
#define DIGIT_WIN_MMVAL_Y_OFS                                                                       \
    (DIGIT_WIN_TXT_Y_OFS + GET_FNOT_H(DIGIT_WIN_FONT) +                                              \
     DEFAULT_DIGIT_WIN_V_S) // 最大最小值y坐标相对边框的偏移
#define DIGIT_WIN_MMVAL_MID_LX(ui)                                                                      \
    (((ui)->fb.w / 2) - (GET_FNOT_W(DIGIT_WIN_NUM_FONT) >> 1)) // mmval中间间隔符左侧的横坐标
#define DIGIT_WIN_MMVAL_MID_RX(ui)                                                                      \
    (((ui)->fb.w / 2) + (GET_FNOT_W(DIGIT_WIN_NUM_FONT) >> 1)) // mmval中间间隔符左侧的横坐标
#define DIGIT_WIN_MMVAL_S DEFAULT_DIGIT_WIN_MMVAL_S // mmval和中间间隔符之间的间隔
#define DIGIT_WIN_MIN_W_MAX(ui)                                                                         \
    (DIGIT_WIN_MMVAL_MID_LX(ui) - DIGIT_WIN_MMVAL_S -                                                    \
     (DIGIT_WIN_X(ui) + DIGIT_WIN_FONT_MARGIN)) // 最小值显示的最大宽度
#define DIGIT_WIN_MAX_W_MAX(ui)                                                                         \
    ((DIGIT_WIN_X(ui) + DIGIT_WIN_W(ui) - DIGIT_WIN_FONT_MARGIN) -                                            \
     (DIGIT_WIN_MMVAL_MID_RX(ui) + DIGIT_WIN_MMVAL_S)) // 最大值显示的最大宽度
#define DIGIT_WIN_NUM_Y_OFS                                                                         \
    (DIGIT_WIN_MMVAL_Y_OFS + GET_FNOT_H(DIGIT_WIN_NUM_FONT) +                                        \
     DEFAULT_DIGIT_WIN_V_S) // 最大最小值y坐标相对边框的偏移
#define DIGIT_WIN_MACRO_ASSERT(ui)                                                                      \
    (DIGIT_WIN_H > (ui)->fb.h ||                                                            \
     (9 * GET_FNOT_W(DIGIT_WIN_NUM_FONT) + 6 * (DIGIT_WIN_NUM_S) > DIGIT_WIN_W(ui)))

//-------微调弹窗的相关方法和函数
static void digit_popup_draw(tm_digit_popup* spw) {
    tm_ui* ui = spw->page.ui;
    char temp_str_buff[DIGIT_WIN_STR_BUFF_SIZE] = {0};
    int16_t num_x = 0;
    // 弹窗外框
    tm_canvas_draw_rbox_empty(&(ui->w_all), DIGIT_WIN_X(ui), spw->vars.win_y.pos_cur,
                               DIGIT_WIN_W(ui), DIGIT_WIN_H, tm_ui_radius(ui, DIGIT_WIN_R));
    // 提示文本跳过“% ”
    spw->vars.text_ss.canvas.h = GET_FNOT_H(DIGIT_WIN_FONT);
    spw->vars.text_ss.canvas.start_y =
        spw->vars.win_y.pos_cur + DIGIT_WIN_FONT_MARGIN;
    spw->vars.text_ss.canvas.w =
        MIN(tm_text_width(spw->text, DIGIT_WIN_FONT), DIGIT_WIN_W(ui) - 2 * DIGIT_WIN_FONT_MARGIN);
    spw->vars.text_ss.canvas.start_x =
        ((ui)->fb.w / 2) - (spw->vars.text_ss.canvas.w >> 1);
    spw->vars.text_ss.str = spw->text;
    tm_slide_str_draw(&(spw->vars.text_ss), 0, DIGIT_WIN_FONT, ui->time);
    // 绘制value range
    spw->vars.min_ss.canvas.h = GET_FNOT_H(DIGIT_WIN_NUM_FONT);
    spw->vars.min_ss.canvas.start_y =
        spw->vars.win_y.pos_cur + DIGIT_WIN_MMVAL_Y_OFS;
    tm_format_general(temp_str_buff, sizeof(temp_str_buff), spw->min, spw->dec_num);
    spw->vars.min_ss.str = temp_str_buff;
    spw->vars.min_ss.canvas.w =
        MIN(tm_text_width(temp_str_buff, DIGIT_WIN_NUM_FONT), DIGIT_WIN_MIN_W_MAX(ui));
    spw->vars.min_ss.canvas.start_x = DIGIT_WIN_MMVAL_MID_LX(ui) - DIGIT_WIN_MMVAL_S -
                                              spw->vars.min_ss.canvas.w; // 靠近中间放置
    tm_canvas_draw_char(&(ui->w_all), DIGIT_WIN_MMVAL_MID_LX(ui),
                           spw->vars.min_ss.canvas.start_y, DIGIT_WIN_NUM_FONT,
                           '~'); // 绘制最大最小值间的字符(居中)
    tm_slide_str_draw(&(spw->vars.min_ss), 0, DIGIT_WIN_NUM_FONT, ui->time);
    spw->vars.max_ss.canvas = spw->vars.min_ss.canvas;
    tm_format_general(temp_str_buff, sizeof(temp_str_buff), spw->max, spw->dec_num);
    spw->vars.max_ss.str = temp_str_buff;
    spw->vars.max_ss.canvas.w =
        MIN(tm_text_width(temp_str_buff, DIGIT_WIN_NUM_FONT), DIGIT_WIN_MAX_W_MAX(ui));
    spw->vars.max_ss.canvas.start_x =
        DIGIT_WIN_MMVAL_MID_RX(ui) + DIGIT_WIN_MMVAL_S; // 靠近中间放置
    tm_slide_str_draw(&(spw->vars.max_ss), 0, DIGIT_WIN_NUM_FONT, ui->time);
    // decimal point
    num_x = ((ui)->fb.w - spw->vars.num_w_temp) >> 1;
    if (TM_DECIMALS_0 != spw->dec_num) // 需要显示小数点
    {
        int16_t dec_point_x_ofs =
            num_x + (8 - (int16_t)(spw->dec_num)) * GET_FNOT_W(DIGIT_WIN_NUM_FONT) +
            (7 - (int16_t)(spw->dec_num)) * DIGIT_WIN_NUM_S; // 减去小数点自身的宽度
        tm_canvas_draw_char(&(ui->w_all), dec_point_x_ofs,
                               spw->vars.win_y.pos_cur + DIGIT_WIN_NUM_Y_OFS,
                               DIGIT_WIN_NUM_FONT, '.');
    }
    snprintf(temp_str_buff, sizeof(temp_str_buff), "%+08" PRId32, spw->val);
    // show numDigit
    for (uint8_t i = 0; i < 8; i++) {
        tm_canvas_draw_char(&(ui->w_all), num_x,
                               spw->vars.win_y.pos_cur + DIGIT_WIN_NUM_Y_OFS,
                               DIGIT_WIN_NUM_FONT, temp_str_buff[i]);
        if (7 - i == (int16_t)spw->dec_num)
            num_x += (GET_FNOT_W(DIGIT_WIN_NUM_FONT) << 1); // 加上小数点的宽度
        else
            num_x += (GET_FNOT_W(DIGIT_WIN_NUM_FONT) + DIGIT_WIN_NUM_S);
    }
}

bool tm_digit_popup_in(tm_page_addr page_addr) {
    tm_digit_popup* self = (tm_digit_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    bool ret = false;
    // 窗口y坐标运动
    tm_animation(&self->vars.win_y, tm_ui_anim_ms(ui, TM_ANI_POPUP), ui->time,
                     &(ui->anim_is_finish));
    // 绘制白色背景
    tm_fb_set_color(&ui->fb, TM_PEN_BLACK);
    tm_canvas_draw_rbox(&(ui->w_all), DIGIT_WIN_X(ui), self->vars.win_y.pos_cur,
                          DIGIT_WIN_W(ui), DIGIT_WIN_H, tm_ui_radius(ui, DIGIT_WIN_R)); // 清空出白色背景
    tm_fb_set_color(&ui->fb, TM_PEN_WHITE);
    digit_popup_draw(self);
    if (self->vars.win_y.pos_cur == self->vars.win_y.pos_tgt)
        ret = true;
    return ret;
}
void tm_digit_popup_in_init(tm_page_addr page_addr) {
    tm_digit_popup* self = (tm_digit_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    self->vars.win_y.pos_tgt = DIGIT_WIN_Y(ui);
    self->vars.win_y.pos_cur = -1 * self->vars.win_y.pos_tgt;
    // self->vars.win_y.pos_tgt = DIGIT_WIN_Y(ui);
    // self->vars.win_y.pos_cur = -1*((ui)->fb.h>>1);
    // 因为digit移动幅度比较小，下降动画不明显，换成一个远一点的起始位置，还是不明显其实🤣
    if (self->auto_get_bg_opt) {
        self->bg_opt = tm_menu_selected(self->page.last_page);
        if (NULL != self->bg_opt) {
            if (NULL != self->bg_opt->text)
                self->text = self->bg_opt->text; // 取出选中项文本
            self->val = self->bg_opt->val;             // 获取背景页面选中项的值
            self->dec_num = self->bg_opt->decimals; // 获取选中项的定点位数
            if (self->val > self->max) {
                self->val = self->max;
                TM_LOG_I("auto get val gt max");
            }
            if (self->val < self->min) {
                self->val = self->min;
                TM_LOG_I("auto get val lt min");
            }
        }
    } else { // 如果没有使能自动获取，且自己还没有设置的话
        if (NULL == self->text)
            self->text = TM_POPUP_TEXT_DEFAULT;
    }
    if (self->dec_num == TM_DECIMALS_0)
        self->vars.num_w_temp = 8 * GET_FNOT_W(DIGIT_WIN_NUM_FONT) + 7 * (DIGIT_WIN_NUM_S);
    else
        self->vars.num_w_temp =
            9 * GET_FNOT_W(DIGIT_WIN_NUM_FONT) + 6 * (DIGIT_WIN_NUM_S); //.和数字之间不需要边距
    self->editing = false; // 一开始进入默认什么都没有选中
    self->vars.text_ss.slide_mode = ui->params.slide_mode;
    self->vars.min_ss.slide_mode = ui->params.slide_mode;
    self->vars.max_ss.slide_mode = ui->params.slide_mode;
    tm_slide_str_reset(&(self->vars.text_ss));
    tm_slide_str_reset(&(self->vars.min_ss));
    tm_slide_str_reset(&(self->vars.max_ss));
    ui->win_bg_blur = ui->params.winbgblur_param[TM_POPUP_BLUR_DIGIT];
}
void tm_digit_popup_show(tm_page_addr page_addr) {
    tm_digit_popup* self = (tm_digit_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    tm_page* bg = (tm_page*)(((tm_page*)page_addr)->last_page);
    tm_page_show(bg); // 先绘制背景
    tm_fb_set_color(&ui->fb, TM_PEN_BLACK);
    tm_fb_blur(&ui->fb, ui->win_bg_blur); // 背景虚化
    tm_canvas_draw_rbox(&(ui->w_all), DIGIT_WIN_X(ui), self->vars.win_y.pos_cur,
                          DIGIT_WIN_W(ui), DIGIT_WIN_H, tm_ui_radius(ui, DIGIT_WIN_R)); // 清空出白色背景
    tm_fb_set_color(&ui->fb, TM_PEN_WHITE);
    digit_popup_draw(self);
    if (ui->anim_is_finish) { // anim结束，且单次滚动没有完成就使能滚动
        if (!self->vars.text_ss.slide_is_finish)
            self->vars.text_ss.slide_enable = true;
        if (!self->vars.min_ss.slide_is_finish)
            self->vars.min_ss.slide_enable = true;
        if (!self->vars.max_ss.slide_is_finish)
            self->vars.max_ss.slide_enable = true;
    }
    // 记录这个页面中所有slide动画的结果&&(同时处理背景中的slide动画)
    ui->slide_is_finish =
        ui->slide_is_finish && !(self->vars.text_ss.slide_enable) &&
        !(self->vars.max_ss.slide_enable) && !(self->vars.min_ss.slide_enable);
}
bool tm_digit_popup_react(tm_page_addr page_addr) {
    tm_digit_popup* self = (tm_digit_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    tm_msg msg = tm_msg_que_read(&(ui->msg_queue));
    bool ret = false;
    if (self->page.auto_input) {
        switch (msg) {
        case TM_MSG_PREV: // 单轴：点击切换选位/改值
        case TM_MSG_NEXT:
            if (self->editing)
                tm_digit_popup_change_digit(self, msg == TM_MSG_NEXT ? 1 : -1);
            else
                tm_digit_popup_move_digit(self, msg == TM_MSG_NEXT);
            break;
        case TM_MSG_LEFT: // 方向键：左右选位，上下直接改值
        case TM_MSG_RIGHT:
            tm_digit_popup_move_digit(self, msg == TM_MSG_RIGHT);
            break;
        case TM_MSG_UP:
        case TM_MSG_DOWN:
            self->editing = true;
            tm_digit_popup_change_digit(self, msg == TM_MSG_UP ? 1 : -1);
            break;
        case TM_MSG_CLICK:
            tm_digit_popup_toggle_editing(self);
            break;
        case TM_MSG_RETURN:
            if (self->auto_set_bg_opt) {
                tm_option* opt = tm_menu_selected(self->page.last_page);
                if (NULL != opt)
                    opt->val = self->val;
            }
            tm_page_return(page_addr);
            self->vars.win_y.pos_tgt = -DIGIT_WIN_H;
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
            if (ret) { // 如果回调函数返回true的话,表示退出这个页面
                tm_page_return(page_addr);
                self->vars.win_y.pos_tgt = -DIGIT_WIN_H;
            }
        }
    }
    return ret;
}
void tm_digit_popup_indicator_ctrl(tm_page_addr page_addr) {
    tm_digit_popup* self = (tm_digit_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    ui->indicator.w.pos_tgt = GET_FNOT_W(DIGIT_WIN_NUM_FONT) + (DIGIT_WIN_NUM_S);
    if (self->selected_digit < (int16_t)self->dec_num)
        ui->indicator.x.pos_tgt = (((ui)->fb.w - self->vars.num_w_temp) >> 1) +
                                        ui->indicator.w.pos_tgt * (7 - self->selected_digit) +
                                        GET_FNOT_W(DIGIT_WIN_NUM_FONT) - DIGIT_WIN_NUM_S -
                                        (DIGIT_WIN_NUM_S >> 1);
    else
        ui->indicator.x.pos_tgt = (((ui)->fb.w - self->vars.num_w_temp) >> 1) +
                                        ui->indicator.w.pos_tgt * (7 - self->selected_digit) -
                                        (DIGIT_WIN_NUM_S >> 1);
    if (self->editing) {
        ui->indicator.y.pos_tgt = DIGIT_WIN_Y(ui) + DIGIT_WIN_NUM_Y_OFS - (DIGIT_WIN_V_S >> 1);
        ui->indicator.h.pos_tgt =
            GET_FNOT_H(DIGIT_WIN_NUM_FONT) + DIGIT_WIN_BOX_H + (DIGIT_WIN_V_S >> 1);
    } else {
        ui->indicator.y.pos_tgt =
            DIGIT_WIN_Y(ui) + DIGIT_WIN_NUM_Y_OFS + GET_FNOT_H(DIGIT_WIN_NUM_FONT);
        ui->indicator.h.pos_tgt = DIGIT_WIN_BOX_H;
    }
    tm_fb_set_color(&ui->fb, 2); // 反色绘制
    tm_canvas_draw_rbox(&(ui->w_all), ui->indicator.x.pos_cur,
                          ui->indicator.y.pos_cur, ui->indicator.w.pos_cur,
                          ui->indicator.h.pos_cur, tm_ui_radius(ui, DIGIT_WIN_BOX_R));
    tm_fb_set_color(&ui->fb, 1); // 实色绘制
}

void tm_digit_popup_set_min_max_decimals(tm_digit_popup* spw, int32_t min, int32_t max,
                                           tm_decimals decnum) {
    if (TM_PAGE_DIGIT_POPUP != tm_page_type_of(spw)) {
        TM_LOG_W("Don't set other type page with min max decimals!");
        return;
    }
    spw->min = MIN(min, max);
    spw->max = MAX(min, max);
    spw->dec_num = decnum;
}
static const tm_page_methods tm_digit_popup_methods = {
    .in = tm_digit_popup_in,
    .in_para_init = tm_digit_popup_in_init,
    .show = tm_digit_popup_show,
    .react = tm_digit_popup_react,
    .indicator_ctrl = tm_digit_popup_indicator_ctrl,
    .scrollbar_ctrl = tm_func_do_nothing,
};

void tm_digit_popup_init(
    tm_digit_popup* spw,
    tm_ui* ui,     // 微调数值弹窗对象指针
    const char* text,       // 显示的文本内容(如果下面两个使能，这个无效)
    int32_t init_val, // 微调数值弹窗结果的初始值(如果下面两个使能，这个无效)
    tm_decimals dec_num, // 微调数值弹窗的定点位数(如果下面两个使能，这个无效)
    int32_t min,        // 弹窗页面的最小值限制
    int32_t max,        // 弹窗页面的最大值限制
    bool auto_get_bg_opt, // 是否自动获取背景菜单页面中的文本
    bool auto_set_bg_opt, // 是否自动赋值背景菜单页面中的文本
    tm_page_cb cb) {
    if (DIGIT_WIN_MACRO_ASSERT(ui))
        TM_LOG_W(
            "The Auto height gt the height of buff,Or The width of num gt the width of DigitWin");
    spw->page.page_type = TM_PAGE_DIGIT_POPUP;
    tm_page_init(spw, ui, cb);
    memset(&spw->vars, 0, sizeof(spw->vars));
    spw->vars.text_ss.slide_mode = ui->params.slide_mode;
    spw->vars.min_ss.slide_mode = ui->params.slide_mode;
    spw->vars.max_ss.slide_mode = ui->params.slide_mode;
    spw->vars.text_ss.canvas.fb = &ui->fb;
    spw->vars.min_ss.canvas.fb = &ui->fb;
    spw->vars.max_ss.canvas.fb = &ui->fb;
    spw->auto_get_bg_opt = auto_get_bg_opt;
    spw->auto_set_bg_opt = auto_set_bg_opt;
    spw->text = text;
    spw->val = init_val;
    spw->selected_digit = 0;
    spw->editing = false;
    tm_digit_popup_set_min_max_decimals(spw, min, max, dec_num);
    spw->bg_opt = NULL;
    spw->page.methods = &tm_digit_popup_methods;
}

void tm_digit_popup_move_digit(tm_digit_popup* spw, bool right) {
    if (right) { // 右移
        if (spw->selected_digit > 0)
            spw->selected_digit--;
    } else { // 左移
        if (spw->selected_digit < 7)
            spw->selected_digit++;
    }
}
void tm_digit_popup_toggle_editing(tm_digit_popup* spw) {
    spw->editing = !spw->editing;
}

bool tm_digit_popup_change_digit(tm_digit_popup* spw, int32_t direction) {
    bool ret = false;
    int32_t val_new = spw->val; // 保存上一次的值，防止超出范围
    int32_t delta_val = direction;
    if (spw->editing) {         // 选中状态才能增加选中位
        if (spw->selected_digit != 7) { // 选中的不是符号位
            for (uint8_t i = 0; i < spw->selected_digit; i++)
                delta_val *= 10;
            val_new += delta_val;
        } else
            val_new = -val_new; // 选中符号位
        if (val_new >= spw->min && val_new <= spw->max) {
            spw->val = val_new;
            ret = true; // 修改成功
        }
    }
    return ret;
}
