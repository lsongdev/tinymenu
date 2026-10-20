#include "core/tm_ui.h"
#include "popups/tm_value_popup.h"
#include "popups/tm_confirm_popup.h" // shares its slide mode default
#include "core/tm_progress.h"

//---------------------------------------------------ValWin页面-----------------------------------------------------------------
// 宏定义
#define VAL_WIN_FONT DEFAULT_VAL_WIN_FONT                   // Val弹窗字体
#define VAL_WIN_STR_BUFF_SIZE DEFAULT_VAL_WIN_STR_BUFF_SIZE // 临时buff的大小
#define VAL_WIN_FONT_MARGIN DEFAULT_VAL_WIN_FONT_MARGIN     // 弹窗内部元素到边框的边距
#define VAL_WIN_SIDE_MIN 4 // 弹窗到屏幕两侧的最小距离
// Val弹窗的宽度：屏幕放不下默认宽度时收窄，收窄的部分由文本区和进度条承担
#define VAL_WIN_W(ui) MIN(DEFAULT_VAL_WIN_W, (ui)->fb.w - 2 * VAL_WIN_SIDE_MIN)
#define VAL_WIN_SHRINK(ui) (DEFAULT_VAL_WIN_W - VAL_WIN_W(ui)) // 比默认宽度窄了多少
#define VAL_WIN_H                                                                                  \
    (2 * GET_FNOT_H(DEFAULT_VAL_WIN_FONT) + 2 * DEFAULT_VAL_WIN_FONT_MARGIN +                      \
     DEFAULT_VAL_TEXT_BAR_S)                                       // val弹窗的高度
#define VAL_WIN_R DEFAULT_VAL_WIN_R                                // Val弹窗的倒角大小
#define VAL_WIN_X(ui) (((ui)->fb.w - VAL_WIN_W(ui)) >> 1)   // 弹窗的起始x
#define VAL_WIN_Y(ui) (((ui)->fb.h - VAL_WIN_H) >> 1)          // 弹窗的起始y
#define VAL_WIN_TXT_W_MAX(ui) (DEFAULT_VAL_WIN_TXT_W_MAX - VAL_WIN_SHRINK(ui))               // txt可显示最大宽度
#define VAL_WIN_TXTVAL_H GET_FNOT_H(DEFAULT_VAL_WIN_FONT)          // text VAL的高度
#define VAL_WIN_TXTVAL_X(ui) (VAL_WIN_X(ui) + DEFAULT_VAL_WIN_FONT_MARGIN) // text val的起始x
#define VAL_WIN_TXTVAL_Y_OFS DEFAULT_VAL_WIN_FONT_MARGIN           // text val相对于弹窗的y偏移
#define VAL_WIN_TXTVAL_S DEFAULT_VAL_WIN_TXTVAL_S // 弹窗内文本和val间的间距
#define VAL_WIN_VAL_W_MAX(ui)                                                                          \
    (VAL_WIN_W(ui) - VAL_WIN_TXT_W_MAX(ui) - VAL_WIN_TXTVAL_S -                                            \
     2 * VAL_WIN_FONT_MARGIN) // val可显示的最大宽度
#define VAL_WIN_MMVAL_W_MAX                                                                        \
    ((DEFAULT_VAL_WIN_W - 2 * DEFAULT_VAL_WIN_FONT_MARGIN - 2 * DEFAULT_VAL_MMVAL_BAR_S -          \
      DEFAULT_VAL_WIN_BAR_W) >>                                                                    \
     1)                                                  // minmaxval可显示最大宽度
#define VAL_WIN_MMVAL_H GET_FNOT_H(DEFAULT_VAL_WIN_FONT) // MINMAXVAL的高度
#define VAL_WIN_MMVAL_Y_OFS                                                                        \
    (VAL_WIN_TXTVAL_Y_OFS + VAL_WIN_TXTVAL_H + DEFAULT_VAL_TEXT_BAR_S) // mmval相对于弹窗的y偏移
#define VAL_WIN_BAR_W(ui) (DEFAULT_VAL_WIN_BAR_W - VAL_WIN_SHRINK(ui))                           // 进度条宽度
#define VAL_WIN_BAR_H DEFAULT_VAL_WIN_BAR_H                            // 进度条高度
#define VAL_WIN_BAR_X(ui) (((ui)->fb.w - VAL_WIN_BAR_W(ui)) >> 1) // 进度条的起始X
#define VAL_WIN_BAR_Y_OFS                                                                          \
    (VAL_WIN_MMVAL_Y_OFS + ((VAL_WIN_MMVAL_H - VAL_WIN_BAR_H) >> 1)) // 进度条相对于边框的Y偏移
#define VAL_WIN_TEXT_BAR_S DEFAULT_VAL_TEXT_BAR_S                    // 下方进度条和上方文本的间距
#define VAL_WIN_TXT_SLISTRMODE DEFAULT_VAL_WIN_TXT_SLISTRMODE        // 文本的移动模式
#define VAL_WIN_VAL_SLISTRMODE DEFAULT_VAL_WIN_VAL_SLISTRMODE        // 数值的移动模式
#define VAL_WIN_MACRO_ASSERT(ui) (VAL_WIN_H > (ui)->fb.h)

//-------数值弹窗的相关方法和函数

static void value_popup_draw(tm_value_popup* vw) {
    tm_ui* ui = vw->page.ui;
    char str_buff[VAL_WIN_STR_BUFF_SIZE] = {0};
    // 弹窗外框
    tm_canvas_draw_rbox_empty(&(ui->w_all), VAL_WIN_X(ui), vw->vars.win_y.pos_cur,
                               VAL_WIN_W(ui), VAL_WIN_H, tm_ui_radius(ui, VAL_WIN_R));
    // 进度条外框(填充部分由指示器绘制)
    tm_progress_draw_frame(ui, VAL_WIN_BAR_X(ui), vw->vars.win_y.pos_cur + VAL_WIN_BAR_Y_OFS,
                           VAL_WIN_BAR_W(ui), VAL_WIN_BAR_H);
    // 绘制提示文本
    vw->vars.text_ss.canvas.start_x = VAL_WIN_TXTVAL_X(ui);
    vw->vars.text_ss.canvas.start_y = vw->vars.win_y.pos_cur + VAL_WIN_TXTVAL_Y_OFS;
    vw->vars.text_ss.canvas.h = VAL_WIN_TXTVAL_H;
    vw->vars.text_ss.str = vw->text;
    vw->vars.text_ss.canvas.w = tm_text_width(vw->text, VAL_WIN_FONT);
    // 绘制数值
    vw->vars.val_ss.canvas = vw->vars.text_ss.canvas;
    tm_format_int(str_buff, sizeof(str_buff), vw->val);
    vw->vars.val_ss.str = str_buff;
    vw->vars.val_ss.canvas.w = tm_text_width(str_buff, VAL_WIN_FONT);
    if (vw->vars.val_ss.canvas.w + vw->vars.text_ss.canvas.w + VAL_WIN_TXTVAL_S >
        VAL_WIN_W(ui) - 2 * VAL_WIN_FONT_MARGIN) {
        // 放不下的话按设置允许的最大长度来
        if (vw->vars.val_ss.canvas.w < VAL_WIN_VAL_W_MAX(ui)) { // 如果值放得下的话，优先值靠右
            vw->vars.text_ss.canvas.w = VAL_WIN_W(ui) - 2 * VAL_WIN_FONT_MARGIN -
                                                vw->vars.val_ss.canvas.w - VAL_WIN_TXTVAL_S;
        } else {
            vw->vars.text_ss.canvas.w = VAL_WIN_TXT_W_MAX(ui);
            vw->vars.val_ss.canvas.w = VAL_WIN_VAL_W_MAX(ui);
            vw->vars.val_ss.canvas.start_x =
                VAL_WIN_TXTVAL_X(ui) + VAL_WIN_TXT_W_MAX(ui) + VAL_WIN_TXTVAL_S;
        }
    }
    // 值靠右放
    vw->vars.val_ss.canvas.start_x =
        VAL_WIN_X(ui) + VAL_WIN_W(ui) - VAL_WIN_FONT_MARGIN - vw->vars.val_ss.canvas.w;
    tm_slide_str_draw(&(vw->vars.text_ss), 0, VAL_WIN_FONT, ui->time);
    tm_slide_str_draw(&(vw->vars.val_ss), 0, VAL_WIN_FONT, ui->time);
    // 绘制最大最小值
    vw->vars.min_ss.canvas.start_y = vw->vars.win_y.pos_cur + VAL_WIN_MMVAL_Y_OFS;
    vw->vars.min_ss.canvas.h = VAL_WIN_MMVAL_H;
    tm_format_int(str_buff, sizeof(str_buff), vw->min);
    vw->vars.min_ss.str = str_buff;
    vw->vars.min_ss.canvas.w =
        MIN(tm_text_width(str_buff, VAL_WIN_FONT), VAL_WIN_MMVAL_W_MAX);
    vw->vars.min_ss.canvas.start_x =
        VAL_WIN_TXTVAL_X(ui) + (VAL_WIN_MMVAL_W_MAX >> 1) - (vw->vars.min_ss.canvas.w >> 1);
    tm_slide_str_draw(&(vw->vars.min_ss), 0, VAL_WIN_FONT, ui->time);
    vw->vars.max_ss.canvas = vw->vars.min_ss.canvas;
    tm_format_int(str_buff, sizeof(str_buff), vw->max);
    vw->vars.max_ss.str = str_buff;
    vw->vars.max_ss.canvas.w =
        MIN(tm_text_width(str_buff, VAL_WIN_FONT), VAL_WIN_MMVAL_W_MAX);
    vw->vars.max_ss.canvas.start_x = VAL_WIN_X(ui) + VAL_WIN_W(ui) - VAL_WIN_FONT_MARGIN -
                                             (VAL_WIN_MMVAL_W_MAX >> 1) -
                                             (vw->vars.max_ss.canvas.w >> 1);
    tm_slide_str_draw(&(vw->vars.max_ss), 0, VAL_WIN_FONT, ui->time);
}

bool tm_value_popup_in(tm_page_addr page_addr) {
    tm_value_popup* self = (tm_value_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    bool ret = false;
    // 窗口y坐标运动
    tm_animation(&self->vars.win_y, tm_ui_anim_ms(ui, TM_ANI_POPUP), ui->time,
                     &(ui->anim_is_finish));
    // 绘制白色背景
    tm_fb_set_color(&ui->fb, TM_PEN_BLACK);
    tm_canvas_draw_rbox(&(ui->w_all), VAL_WIN_X(ui), self->vars.win_y.pos_cur, VAL_WIN_W(ui),
                          VAL_WIN_H, tm_ui_radius(ui, VAL_WIN_R)); // 清空出白色背景
    tm_fb_set_color(&ui->fb, TM_PEN_WHITE);
    value_popup_draw(self);
    if (self->vars.win_y.pos_cur == self->vars.win_y.pos_tgt)
        ret = true;

    return ret;
}
void tm_value_popup_in_init(tm_page_addr page_addr) {
    tm_value_popup* self = (tm_value_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    self->vars.win_y.pos_tgt = VAL_WIN_Y(ui);
    self->vars.win_y.pos_cur = -self->vars.win_y.pos_tgt;
    if (self->auto_get_bg_opt) {
        self->bg_opt = tm_menu_selected(self->page.last_page);
        if (NULL != self->bg_opt) {
            if (NULL != self->bg_opt->text)
                self->text = self->bg_opt->text; // 取出选中项文本
            self->val = self->bg_opt->val;             // 获取背景页面选中项的值
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
    self->vars.text_ss.slide_mode = ui->params.slide_mode;
    self->vars.val_ss.slide_mode = ui->params.slide_mode;
    self->vars.min_ss.slide_mode = ui->params.slide_mode;
    self->vars.max_ss.slide_mode = ui->params.slide_mode;
    tm_slide_str_reset(&(self->vars.text_ss));
    tm_slide_str_reset(&(self->vars.val_ss));
    tm_slide_str_reset(&(self->vars.min_ss));
    tm_slide_str_reset(&(self->vars.max_ss));
    ui->win_bg_blur = ui->params.winbgblur_param[TM_POPUP_BLUR_VALUE];
}
void tm_value_popup_show(tm_page_addr page_addr) {
    tm_value_popup* self = (tm_value_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    tm_page* bg = (tm_page*)(((tm_page*)page_addr)->last_page);
    tm_page_show(bg); // 先绘制背景
    tm_fb_set_color(&ui->fb, TM_PEN_BLACK);
    tm_fb_blur(&ui->fb, ui->win_bg_blur); // 背景虚化
    tm_canvas_draw_rbox(&(ui->w_all), VAL_WIN_X(ui), self->vars.win_y.pos_cur, VAL_WIN_W(ui),
                          VAL_WIN_H, tm_ui_radius(ui, VAL_WIN_R)); // 清空出白色背景
    tm_fb_set_color(&ui->fb, TM_PEN_WHITE);
    value_popup_draw(self);
    if (ui->anim_is_finish) { // anim结束，且单次滚动没有完成就使能滚动
        if (!(self->vars.text_ss.slide_is_finish))
            self->vars.text_ss.slide_enable = true;
        if (!(self->vars.val_ss.slide_is_finish))
            self->vars.val_ss.slide_enable = true;
        if (!(self->vars.max_ss.slide_is_finish))
            self->vars.max_ss.slide_enable = true;
        if (!(self->vars.min_ss.slide_is_finish))
            self->vars.min_ss.slide_enable = true;
    }
    // 记录这个页面中所有slide动画的结果&&(同时处理背景中的slide动画)
    ui->slide_is_finish =
        ui->slide_is_finish && !(self->vars.text_ss.slide_enable) &&
        !(self->vars.val_ss.slide_enable) && !(self->vars.max_ss.slide_enable) &&
        !(self->vars.min_ss.slide_enable);
}
bool tm_value_popup_react(tm_page_addr page_addr) {
    tm_value_popup* self = (tm_value_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    bool ret = false;
    tm_msg msg = tm_msg_que_read(&(ui->msg_queue));
    if (self->page.auto_input) {
        switch (msg) {
        case TM_MSG_PREV:
        case TM_MSG_LEFT:
        case TM_MSG_DOWN: // 上一个/左/下：减少
            tm_value_popup_step(self, -1);
            break;
        case TM_MSG_NEXT:
        case TM_MSG_RIGHT:
        case TM_MSG_UP: // 下一个/右/上：增加
            tm_value_popup_step(self, 1);
            break;
        case TM_MSG_CLICK:
            if (self->auto_set_bg_opt) { // 如果使能自动赋值bg页面选中项值的话
                tm_option* opt = tm_menu_selected(self->page.last_page);
                if (NULL != opt)
                    opt->val = self->val;
            }
            // fall through - a click also closes the popup
        case TM_MSG_RETURN:
            tm_page_return(page_addr);
            self->vars.win_y.pos_tgt = -1 * VAL_WIN_H;
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
            if (ret) { // 回调函数返回true表示退出
                tm_page_return(page_addr);
                self->vars.win_y.pos_tgt = -1 * VAL_WIN_H;
            }
        }
    }
    return ret;
}
void tm_value_popup_indicator_ctrl(tm_page_addr page_addr) {
    tm_value_popup* self = (tm_value_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    // 进度条的填充由指示器绘制：打开时选中框会飞进进度条
    ui->indicator.x.pos_tgt = VAL_WIN_BAR_X(ui) + 2;                 // 外框宽占1
    ui->indicator.y.pos_tgt = VAL_WIN_Y(ui) + VAL_WIN_BAR_Y_OFS + 2; // 外框宽占1
    ui->indicator.w.pos_tgt = tm_progress_fill_width(self->min, self->max, self->val, VAL_WIN_BAR_W(ui));
    ui->indicator.h.pos_tgt = VAL_WIN_BAR_H - 4;
    tm_progress_draw_fill(ui, ui->indicator.x.pos_cur, ui->indicator.y.pos_cur,
                          ui->indicator.w.pos_cur, ui->indicator.h.pos_cur);
}

void tm_value_popup_set_min_step_max(tm_value_popup* vw, int32_t min, int32_t step, int32_t max) {
    if (TM_PAGE_VALUE_POPUP != tm_page_type_of(vw)) {
        TM_LOG_W("Don't set other type page with min step max!");
        return;
    }
    vw->min = MIN(min, max);
    vw->max = MAX(min, max);
    vw->step = ABS(step);
}
static const tm_page_methods tm_value_popup_methods = {
    .in = tm_value_popup_in,
    .in_para_init = tm_value_popup_in_init,
    .show = tm_value_popup_show,
    .react = tm_value_popup_react,
    .indicator_ctrl = tm_value_popup_indicator_ctrl,
    .scrollbar_ctrl = tm_func_do_nothing,
};

void tm_value_popup_init(tm_value_popup* vw,
    tm_ui* ui, // 数值弹窗对象指针
                           const char* text, // 显示的文本内容(如)
                           int32_t init_val, // 数值弹窗结果的初始值(如果下面两个使能，这个无效)
                           int32_t min,          // 弹窗页面的最小值限制
                           int32_t max,          // 弹窗页面的最大值限制
                           int32_t step,         // 弹窗页面滑动条的增减步长
                           bool auto_get_bg_opt, // 是否自动获取背景菜单页面中的文本
                           bool auto_set_bg_opt, // 是否自动赋值背景菜单页面中的文本
                           tm_page_cb cb) {
    if (VAL_WIN_MACRO_ASSERT(ui))
        TM_LOG_W("the font /the margin of val_win is too large");
    vw->page.page_type = TM_PAGE_VALUE_POPUP;
    tm_page_init(vw, ui, cb);
    memset(&vw->vars, 0, sizeof(vw->vars));
    vw->vars.text_ss.slide_mode = ui->params.slide_mode;
    vw->vars.val_ss.slide_mode = ui->params.slide_mode;
    vw->vars.min_ss.slide_mode = ui->params.slide_mode;
    vw->vars.max_ss.slide_mode = ui->params.slide_mode;
    vw->vars.text_ss.canvas.fb = &ui->fb;
    vw->vars.val_ss.canvas.fb = &ui->fb;
    vw->vars.min_ss.canvas.fb = &ui->fb;
    vw->vars.max_ss.canvas.fb = &ui->fb;
    vw->auto_get_bg_opt = auto_get_bg_opt;
    vw->auto_set_bg_opt = auto_set_bg_opt;
    vw->text = text;
    tm_value_popup_set_min_step_max(vw, min, step, max);
    vw->val = init_val;
    vw->bg_opt = NULL;
    vw->page.methods = &tm_value_popup_methods;
}
// valwin相关接口函数
bool tm_value_popup_step(tm_value_popup* vw, int8_t dir) {
    const int32_t to = vw->val + (dir > 0 ? vw->step : -vw->step);
    if (dir == 0 || to > vw->max || to < vw->min)
        return false;
    vw->val = to;
    return true;
}
