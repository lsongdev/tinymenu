#include "core/tm_ui.h"
#include "popups/tm_confirm_popup.h"

//-----------------------------------------------ConfWin页面-----------------------------------------------------------
// 宏参数
#define CONF_WIN_FONT DEFAULT_CONF_WIN_FONT                                // 弹窗字体
#define CONF_WIN_W(ui) MIN(DEFAULT_CONF_WIN_W, (ui)->fb.w - 8) // 弹窗宽度(屏幕窄时收窄)
#define CONF_WIN_HORIZON_S(ui) (((ui)->fb.w - CONF_WIN_W(ui)) >> 1) // 弹窗水平上与两侧的边距
// 弹窗最大高度：上下至少留 DEFAULT_CONF_WIN_V_S_MIN，矮屏上按屏高的 1/5 留
#define CONF_WIN_H_MAX(ui) ((ui)->fb.h - 2 * MIN(DEFAULT_CONF_WIN_V_S_MIN, (ui)->fb.h / 5))
#define CONF_WIN_R DEFAULT_CONF_WIN_R                                      // 弹窗倒角
#define CONF_WIN_FONT_MARGIN DEFAULT_CONF_WIN_FONT_MARGIN // CONF弹窗文字到框的边距
#define CONF_WIN_IND_BTN_S DEFAULT_CONF_WIN_IND_BTN_S // 弹窗的按键内文字到边框的距离
#define CONF_WIN_BTN_H (GET_FNOT_H(CONF_WIN_FONT) + 2 * DEFAULT_CONF_WIN_IND_BTN_S) // 按键高度
#define CONF_WIN_BTN_R DEFAULT_CONF_WIN_BTN_R                                       // 按键倒角
#define CONF_TEXT_BTN_S DEFAULT_CONF_WIN_TEXT_BTN_S // 文本和按键的竖直间距
#define CONF_BTN_MAX_W(ui)                                                                             \
    (((ui)->fb.w / 2) - CONF_WIN_HORIZON_S(ui) - 2 * DEFAULT_CONF_WIN_FONT_MARGIN) // 按键文本的最大宽度

//-----ConfWin相关方法和函数

// 抽象出这个函数主要因为在IN和show中有大部分重合的绘制语句
static void confirm_popup_draw(tm_confirm_popup* cw) {
    tm_ui* ui = cw->page.ui;
    // 绘制外框
    tm_canvas_draw_rbox_empty(
        &(ui->w_all), CONF_WIN_HORIZON_S(ui), cw->vars.win_y.pos_cur, CONF_WIN_W(ui),
        cw->vars.canvas.h + CONF_WIN_BTN_H + 2 * CONF_WIN_FONT_MARGIN + CONF_TEXT_BTN_S,
        tm_ui_radius(ui, CONF_WIN_R));
    if (NULL != cw->content) // 绘制提示文本
        tm_canvas_draw_wrapped(&(cw->vars.canvas), 0, cw->str_start_y,
                                        CONF_WIN_FONT, cw->content);
    cw->vars.btn_ss.canvas.start_y =
        cw->vars.canvas.start_y + cw->vars.canvas.h + CONF_TEXT_BTN_S;
    tm_slide_str_draw(&(cw->vars.btn_ss), CONF_WIN_IND_BTN_S,
                              CONF_WIN_FONT, ui->time); // 绘制选中项文本
    // 绘制另一个选项
    tm_canvas temp_canvas = cw->vars.btn_ss.canvas;
    if (cw->conf_ret) { // 选中左边，绘制右边
        temp_canvas.w = MIN(tm_text_width(cw->str_right, CONF_WIN_FONT), CONF_BTN_MAX_W(ui));
        temp_canvas.start_x = ((ui)->fb.w / 2) + (CONF_WIN_W(ui) >> 2) - (temp_canvas.w >> 1);
        tm_canvas_draw_str(&temp_canvas, 0, CONF_WIN_IND_BTN_S, CONF_WIN_FONT,
                             (cw->str_right));
    } else {
        temp_canvas.w = MIN(tm_text_width(cw->str_left, CONF_WIN_FONT), CONF_BTN_MAX_W(ui));
        temp_canvas.start_x = CONF_WIN_HORIZON_S(ui) + (CONF_WIN_W(ui) >> 2) - (temp_canvas.w >> 1);
        tm_canvas_draw_str(&temp_canvas, 0, CONF_WIN_IND_BTN_S, CONF_WIN_FONT,
                             (cw->str_left));
    }
}

bool tm_confirm_popup_in(tm_page_addr page_addr) {
    tm_confirm_popup* self = (tm_confirm_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    bool ret = false;
    tm_animation(&(self->vars.win_y), tm_ui_anim_ms(ui, TM_ANI_POPUP), ui->time,
                     &(ui->anim_is_finish));
    // 绘制白色背景
    self->vars.canvas.start_y = self->vars.win_y.pos_cur + CONF_WIN_FONT_MARGIN;
    tm_fb_set_color(&ui->fb, TM_PEN_BLACK);
    tm_canvas_draw_rbox(
        &(ui->w_all), CONF_WIN_HORIZON_S(ui), self->vars.win_y.pos_cur, CONF_WIN_W(ui),
        self->vars.canvas.h + CONF_WIN_BTN_H + 2 * CONF_WIN_FONT_MARGIN + CONF_TEXT_BTN_S,
        tm_ui_radius(ui, CONF_WIN_R)); // 清空出白色背景
    tm_fb_set_color(&ui->fb, TM_PEN_WHITE);
    confirm_popup_draw(self); // 绘制弹窗页面内部的元素
    if (self->vars.win_y.pos_cur == self->vars.win_y.pos_tgt)
        ret = true;
    return ret;
}
void tm_confirm_popup_in_init(tm_page_addr page_addr) {
    tm_confirm_popup* self = (tm_confirm_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    if (self->auto_get_bg_opt) {
        self->bg_opt = tm_menu_selected(self->page.last_page);
        if (NULL != self->bg_opt) {
            if (NULL != self->bg_opt->content)
                self->content = self->bg_opt->content; // 取出选中项的内容
            else if (NULL != self->bg_opt->text)
                self->content = self->bg_opt->text; // 如果选中项没有内容文本，显示标题文本
            self->conf_ret = self->bg_opt->val; // 获取背景页面选中项的选中与否
        }
    } else { // 如果没有使能自动获取，且自己还没有设置的话
        if (NULL == self->content)
            self->content = TM_POPUP_TEXT_DEFAULT;
    }
    self->str_start_y = 0; // 显示进度清零
    // 计算文本框高度
    if (NULL != self->content)
        self->vars.autonewline_need_h = tm_text_height_wrapped(
            CONF_WIN_W(ui) - 2 * CONF_WIN_FONT_MARGIN, self->content, CONF_WIN_FONT);
    else
        self->vars.autonewline_need_h =
            GET_FNOT_H(CONF_WIN_FONT); // 如果是NULL的话，文本框高度为单字符高度
    self->vars.canvas.h =
        MIN(self->vars.autonewline_need_h,
            CONF_WIN_H_MAX(ui) - 2 * CONF_WIN_FONT_MARGIN - CONF_WIN_BTN_H - CONF_TEXT_BTN_S);
    self->vars.canvas.w = CONF_WIN_W(ui) - 2 * CONF_WIN_FONT_MARGIN;
    self->vars.canvas.start_x = CONF_WIN_HORIZON_S(ui) + CONF_WIN_FONT_MARGIN;
    self->vars.btn_ss.canvas.h = CONF_WIN_BTN_H;
    if (self->conf_ret) {
        self->vars.btn_ss.str = self->str_left;
        self->vars.btn_ss.canvas.w =
            MIN(tm_text_width(self->str_left, CONF_WIN_FONT), CONF_BTN_MAX_W(ui));
        self->vars.btn_ss.canvas.start_x =
            CONF_WIN_HORIZON_S(ui) + (CONF_WIN_W(ui) >> 2) - (self->vars.btn_ss.canvas.w >> 1);
    } else {
        self->vars.btn_ss.str = self->str_right;
        self->vars.btn_ss.canvas.w =
            MIN(tm_text_width(self->str_right, CONF_WIN_FONT), CONF_BTN_MAX_W(ui));
        self->vars.btn_ss.canvas.start_x =
            ((ui)->fb.w / 2) + (CONF_WIN_W(ui) >> 2) - (self->vars.btn_ss.canvas.w >> 1);
    }
    self->vars.win_y.pos_tgt =
        (((ui)->fb.h - self->vars.canvas.h - self->vars.btn_ss.canvas.h -
          2 * CONF_WIN_FONT_MARGIN - CONF_TEXT_BTN_S) >>
         1);
    self->vars.win_y.pos_cur = -self->vars.win_y.pos_tgt;
    self->vars.btn_ss.slide_mode = ui->params.slide_mode;
    tm_slide_str_reset(&(self->vars.btn_ss));
    ui->win_bg_blur = ui->params.winbgblur_param[TM_POPUP_BLUR_CONFIRM];
}
void tm_confirm_popup_show(tm_page_addr page_addr) {
    tm_confirm_popup* self = (tm_confirm_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    tm_page* bg = (tm_page*)(((tm_page*)page_addr)->last_page);
    tm_page_show(bg); // 先绘制背景
    tm_fb_set_color(&ui->fb, TM_PEN_BLACK);
    tm_fb_blur(&ui->fb, ui->win_bg_blur); // 背景虚化
    tm_canvas_draw_rbox(
        &(ui->w_all), CONF_WIN_HORIZON_S(ui), self->vars.win_y.pos_cur, CONF_WIN_W(ui),
        self->vars.canvas.h + CONF_WIN_BTN_H + 2 * CONF_WIN_FONT_MARGIN + CONF_TEXT_BTN_S,
        tm_ui_radius(ui, CONF_WIN_R)); // 清空出白色背景
    tm_fb_set_color(&ui->fb, TM_PEN_WHITE);
    confirm_popup_draw(self);
    if (ui->anim_is_finish &&
        !(self->vars.btn_ss.slide_is_finish)) { // anim结束，且单次滚动没有完成就使能滚动
        self->vars.btn_ss.slide_enable = true;
    }
    // ui->slide_is_finish = !(self->vars.btn_ss.slide_enable);
    // //这个统计必须放show函数最后
    ui->slide_is_finish =
        ui->slide_is_finish && !(self->vars.btn_ss.slide_enable);
    // 在弹窗中&&上自身是因为自身存着背景中slide动画是否完成的标志
    // 和anim_is_finish的标志一样在show阶段最开始置true，在show最后验收给motionless的标志位
}
bool tm_confirm_popup_react(tm_page_addr page_addr) {
    tm_confirm_popup* self = (tm_confirm_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    bool ret = false; // 返回true时会触发UI切换状态
    tm_msg msg = tm_msg_que_read(&(ui->msg_queue));
    if (self->page.auto_input) {
        switch (msg) {
        case TM_MSG_NEXT:
        case TM_MSG_DOWN: // 先把文本滚完，再切换按钮(单轴设备也能选到两个按钮)
            if (!tm_confirm_popup_slide_up_txt(self))
                tm_confirm_popup_toggle_btn(self);
            break;
        case TM_MSG_PREV:
        case TM_MSG_UP:
            if (!tm_confirm_popup_slide_down_txt(self))
                tm_confirm_popup_toggle_btn(self);
            break;
        case TM_MSG_LEFT:
        case TM_MSG_RIGHT:
            tm_confirm_popup_toggle_btn(self);
            break;
        case TM_MSG_CLICK:
            if (self->auto_set_bg_opt) { // 如果使能自动赋值bg页面选中项值的话
                tm_option* opt = tm_menu_selected(self->page.last_page);
                if (NULL != opt)
                    opt->val = self->conf_ret;
            }
            // fall through - a click also closes the popup
        case TM_MSG_RETURN:
            tm_page_return(page_addr);
            self->vars.win_y.pos_tgt = -CONF_WIN_H_MAX(ui);
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
            if (ret) { // 回调函数返回true表示退出页面
                tm_page_return(page_addr);
                self->vars.win_y.pos_tgt = -CONF_WIN_H_MAX(ui);
            }
        }
    }
    return ret;
}
void tm_confirm_popup_indicator_ctrl(tm_page_addr page_addr) {
    tm_confirm_popup* self = (tm_confirm_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    UNUSED_PARAMETER(page_addr);
    ui->indicator.x.pos_tgt = self->vars.btn_ss.canvas.start_x - CONF_WIN_IND_BTN_S;
    ui->indicator.w.pos_tgt = self->vars.btn_ss.canvas.w + 2 * CONF_WIN_IND_BTN_S;
    ui->indicator.h.pos_tgt = self->vars.btn_ss.canvas.h;
    // 因为btnss的start_y是会随win_y运动的
    ui->indicator.y.pos_tgt = self->vars.win_y.pos_tgt + CONF_WIN_FONT_MARGIN +
                                    self->vars.canvas.h + CONF_TEXT_BTN_S;
    tm_ui_draw_highlight(ui, CONF_WIN_BTN_R);
}

static const tm_page_methods tm_confirm_popup_methods = {
    .in = tm_confirm_popup_in,
    .in_para_init = tm_confirm_popup_in_init,
    .show = tm_confirm_popup_show,
    .react = tm_confirm_popup_react,
    .indicator_ctrl = tm_confirm_popup_indicator_ctrl,
    .scrollbar_ctrl = tm_func_do_nothing,
};

void tm_confirm_popup_init(tm_confirm_popup* cw,
    tm_ui* ui,     // 确认弹窗对象指针
                            const char* content,   // 显示的文本内容
                            const char* str_left,  // 左按键的文本
                            const char* str_right, // 右按键的文本
                            bool conf_ret, // 确认弹窗结果的初始值(如果下面两个使能，这个无效)
                            bool auto_get_bg_opt, // 是否自动获取背景菜单页面中的文本
                            bool auto_set_bg_opt, // 是否自动赋值背景菜单页面中的文本
                            uint16_t move_step,   // 文本移动时的调整步长
                            tm_page_cb cb)      // 回调函数
{
    cw->page.page_type = TM_PAGE_CONFIRM_POPUP;
    tm_page_init(cw, ui, cb);
    memset(&cw->vars, 0, sizeof(cw->vars));
    cw->vars.btn_ss.slide_mode = ui->params.slide_mode;
    cw->vars.canvas.fb = &ui->fb;
    cw->vars.btn_ss.canvas.fb = &ui->fb;
    cw->auto_get_bg_opt = auto_get_bg_opt;
    cw->auto_set_bg_opt = auto_set_bg_opt;
    cw->content = content;
    cw->conf_ret = conf_ret;
    if (NULL == str_left)
        cw->str_left = "Yes";
    else
        cw->str_left = str_left;
    if (NULL == str_right)
        cw->str_right = "No";
    else
        cw->str_right = str_right;
    cw->str_start_y = 0;
    cw->str_move_step = move_step;
    cw->bg_opt = NULL;
    cw->page.methods = &tm_confirm_popup_methods;
}
// COnfwin相关接口函数
bool tm_confirm_popup_slide_up_txt(tm_confirm_popup* cw) {
    tm_ui* ui = cw->page.ui;
    bool ret = false;
    if (cw->vars.autonewline_need_h + cw->str_start_y >
        CONF_WIN_H_MAX(ui) - 2 * CONF_WIN_FONT_MARGIN - CONF_WIN_BTN_H - CONF_TEXT_BTN_S) {
        cw->str_start_y -= cw->str_move_step;
        ret = true;
    }
    return ret;
}
bool tm_confirm_popup_slide_down_txt(tm_confirm_popup* cw) {
    bool ret = false;
    if (cw->str_start_y < 0) {
        cw->str_start_y += cw->str_move_step;
        ret = true;
    }
    return ret;
}
void tm_confirm_popup_toggle_btn(tm_confirm_popup* cw) {
    tm_ui* ui = cw->page.ui;
    tm_slide_str_reset(&(cw->vars.btn_ss));
    cw->conf_ret = !cw->conf_ret;
    if (cw->conf_ret) {
        cw->vars.btn_ss.str = cw->str_left;
        cw->vars.btn_ss.canvas.w =
            MIN(tm_text_width(cw->str_left, CONF_WIN_FONT), CONF_BTN_MAX_W(ui));
        cw->vars.btn_ss.canvas.start_x =
            CONF_WIN_HORIZON_S(ui) + (CONF_WIN_W(ui) >> 2) - (cw->vars.btn_ss.canvas.w >> 1);
    } else {
        cw->vars.btn_ss.str = cw->str_right;
        cw->vars.btn_ss.canvas.w =
            MIN(tm_text_width(cw->str_right, CONF_WIN_FONT), CONF_BTN_MAX_W(ui));
        cw->vars.btn_ss.canvas.start_x =
            ((ui)->fb.w / 2) + (CONF_WIN_W(ui) >> 2) - (cw->vars.btn_ss.canvas.w >> 1);
    }
}
