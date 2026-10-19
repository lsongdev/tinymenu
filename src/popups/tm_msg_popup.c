#include "core/tm_ui.h"
#include "popups/tm_msg_popup.h"

//-------------------------------------MagWin页面----------------------------------------------
// 宏参数
#define MSG_WIN_FONT DEFAULT_MSG_WIN_FONT                                // 弹窗字体
#define MSG_WIN_W(ui) MIN(DEFAULT_MSG_WIN_W, (ui)->fb.w - 8) // 弹窗宽度(屏幕窄时收窄)
// 弹窗最大高度：上下至少留 DEFAULT_MSG_WIN_V_S_MIN，矮屏上按屏高的 1/5 留
#define MSG_WIN_H_MAX(ui) ((ui)->fb.h - 2 * MIN(DEFAULT_MSG_WIN_V_S_MIN, (ui)->fb.h / 5))
#define MSG_WIN_R DEFAULT_MSG_WIN_R                                      // 弹窗倒角
#define MSG_WIN_FONT_MARGIN DEFAULT_MSG_WIN_FONT_MARGIN // MSG弹窗文字到框的边距

//-----MsgWin相关方法和函数
bool tm_msg_popup_in(
    tm_page_addr
        page_addr) // 进入页面的过度动画函数(返回值为True表示这个状态已经可以结束了，切换下一个状态)
{
    tm_msg_popup* self = (tm_msg_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    bool ret = false;
    tm_fb_set_color(&ui->fb, TM_PEN_BLACK);
    if (ui->state == TM_STATE_IN)
        tm_canvas_draw_rbox(&(ui->w_all), ui->indicator.x.pos_cur,
                              ui->indicator.y.pos_cur, ui->indicator.w.pos_cur,
                              ui->indicator.h.pos_cur, tm_ui_radius(ui, MSG_WIN_R)); // 清空出白色背景
    tm_fb_set_color(&ui->fb, TM_PEN_WHITE);
    // 绘制部分和show基本重合，因为语句不多，就不单独抽成函数了
    self->vars.canvas.start_x = ui->indicator.x.pos_cur + MSG_WIN_FONT_MARGIN;
    self->vars.canvas.start_y = ui->indicator.y.pos_cur + MSG_WIN_FONT_MARGIN;
    self->vars.canvas.h = ui->indicator.h.pos_cur - 2 * MSG_WIN_FONT_MARGIN;
    self->vars.canvas.w = ui->indicator.w.pos_cur - 2 * MSG_WIN_FONT_MARGIN;
    if (NULL != self->content && ui->state == TM_STATE_IN)
        tm_canvas_draw_wrapped(&(self->vars.canvas), 0, self->str_start_y,
                                        MSG_WIN_FONT, self->content);
    // msgwin的文字动画是随着指示器运动的，所以这里直接判断指示器是否完成与否即可
    if (ui->indicator.x.pos_cur == ui->indicator.x.pos_tgt &&
        ui->indicator.h.pos_cur == ui->indicator.h.pos_tgt &&
        ui->indicator.w.pos_cur == ui->indicator.w.pos_tgt &&
        ui->indicator.y.pos_cur == ui->indicator.y.pos_tgt)
        ret = true;
    return ret; // 通知上层需要切换状态了
}
void tm_msg_popup_in_init(tm_page_addr page_addr) // 进入页面过度动画参数初始化函数
{
    tm_msg_popup* self = (tm_msg_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    if (self->auto_get_bg_opt) {
        self->bg_opt = tm_menu_selected(self->page.last_page);
        if (NULL != self->bg_opt) {
            if (NULL != self->bg_opt->content)
                self->content = self->bg_opt->content; // 取出选中项的内容
            else if (NULL != self->bg_opt->text)
                self->content = self->bg_opt->text; // 如果选中项没有内容文本，显示标题文本
        }
    } else { // 如果没有使能自动获取，且自己还没有设置的话
        if (NULL == self->content)
            self->content = TM_POPUP_TEXT_DEFAULT;
    }
    self->vars.autonewline_need_h = tm_text_height_wrapped(
        MSG_WIN_W(ui) - 2 * MSG_WIN_FONT_MARGIN, self->content, MSG_WIN_FONT);
    self->vars.indicator_h_temp =
        MIN(self->vars.autonewline_need_h + 2 * MSG_WIN_FONT_MARGIN, MSG_WIN_H_MAX(ui));
    self->str_start_y = 0;                                               // 显示进度清零
    ui->win_bg_blur = ui->params.winbgblur_param[TM_POPUP_BLUR_MSG]; // 设置该弹窗背景的虚化程度
}
void tm_msg_popup_show(tm_page_addr page_addr) // 页面的展示函数
{
    tm_msg_popup* self = (tm_msg_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    tm_page* bg = (tm_page*)(((tm_page*)page_addr)->last_page);
    tm_page_show(bg); // 先绘制背景
    tm_fb_set_color(&ui->fb, TM_PEN_BLACK);
    tm_fb_blur(&ui->fb, ui->win_bg_blur); // 背景虚化
    tm_canvas_draw_rbox(&(ui->w_all), ui->indicator.x.pos_cur,
                          ui->indicator.y.pos_cur, ui->indicator.w.pos_cur,
                          ui->indicator.h.pos_cur, tm_ui_radius(ui, MSG_WIN_R)); // 清空出白色背景
    tm_fb_set_color(&ui->fb, TM_PEN_WHITE);
    self->vars.canvas.start_x = ui->indicator.x.pos_cur + MSG_WIN_FONT_MARGIN;
    self->vars.canvas.start_y = ui->indicator.y.pos_cur + MSG_WIN_FONT_MARGIN;
    self->vars.canvas.h = ui->indicator.h.pos_cur - 2 * MSG_WIN_FONT_MARGIN;
    self->vars.canvas.w = ui->indicator.w.pos_cur - 2 * MSG_WIN_FONT_MARGIN;
    if (NULL != self->content)
        tm_canvas_draw_wrapped(&(self->vars.canvas), 0, self->str_start_y,
                                        MSG_WIN_FONT, self->content);
}
bool tm_msg_popup_react(
    tm_page_addr page_addr) // 页面的响应函数(返回值为True表示这个状态已经可以结束了，切换下一个状态)
{
    tm_msg_popup* self = (tm_msg_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    bool ret = false; // 返回true时会触发UI切换状态
    tm_msg msg = tm_msg_que_read(&(ui->msg_queue));
    if (self->page.auto_input) {
        switch (msg) {
        case TM_MSG_NEXT:
        case TM_MSG_RIGHT:
        case TM_MSG_DOWN:
            tm_msg_popup_slide_up_txt(self);
            break;
        case TM_MSG_PREV:
        case TM_MSG_LEFT:
        case TM_MSG_UP:
            tm_msg_popup_slide_down_txt(self);
            break;
        case TM_MSG_RETURN:
        case TM_MSG_CLICK:
            tm_page_return(page_addr);
            ret = true;
            break;
        default:
            break;
        }
        if (TM_MSG_NONE != msg && NULL != self->page.cb)
            self->page.cb(&(self->page), msg); // 任何输入的有效信息都会调用回调函数
    } else {
        if (TM_MSG_NONE != msg && NULL != self->page.cb) { // 任何输入的有效信息都会调用回调函数
            ret = self->page.cb(&(self->page), msg); // 返回ture表示退出这个页面
            if (ret)
                tm_page_return(page_addr);
        }
    }
    return ret;
}
void tm_msg_popup_indicator_ctrl(tm_page_addr page_addr) // 页面与UI指示器交互的控制函数
{
    tm_msg_popup* self = (tm_msg_popup*)page_addr;
    tm_ui* ui = self->page.ui;
    ui->indicator.w.pos_tgt = MSG_WIN_W(ui);
    if (NULL == self->content)
        ui->indicator.h.pos_tgt = GET_FNOT_H(MSG_WIN_FONT); // 文本为空的话，弹窗高度为单字符
    else {
        ui->indicator.h.pos_tgt = self->vars.indicator_h_temp;
    }
    ui->indicator.x.pos_tgt = ((ui)->fb.w - MSG_WIN_W(ui)) / 2;
    ui->indicator.y.pos_tgt = ((ui)->fb.h - ui->indicator.h.pos_tgt) / 2;
    tm_canvas_draw_rbox_empty(&(ui->w_all), ui->indicator.x.pos_cur,
                               ui->indicator.y.pos_cur, ui->indicator.w.pos_cur,
                               ui->indicator.h.pos_cur, tm_ui_radius(ui, MSG_WIN_R));
}

static const tm_page_methods tm_msg_popup_methods = {
    .in = tm_msg_popup_in,
    .in_para_init = tm_msg_popup_in_init,
    .show = tm_msg_popup_show,
    .react = tm_msg_popup_react,
    .indicator_ctrl = tm_msg_popup_indicator_ctrl,
    .scrollbar_ctrl = tm_func_do_nothing,
};

void tm_msg_popup_init(tm_msg_popup* mw,
    tm_ui* ui,           // 消息弹窗对象指针
                           const char* content,        // 显示的文本内容
                           bool auto_get_bg_opt, // 是否自动获取背景菜单页面中的文本
                           uint16_t move_step,   // 文本移动时的调整步长
                           tm_page_cb cb)      // 回调函数
{
    mw->page.page_type = TM_PAGE_MSG_POPUP;
    tm_page_init(mw, ui, cb);
    memset(&mw->vars, 0, sizeof(mw->vars));
    mw->vars.canvas.fb = &ui->fb;
    mw->auto_get_bg_opt = auto_get_bg_opt;
    mw->content = content;
    mw->str_start_y = 0;
    mw->str_move_step = move_step;
    mw->bg_opt = NULL;
    mw->page.methods = &tm_msg_popup_methods;
}
// MsgWin的接口函数
bool tm_msg_popup_set_content(tm_msg_popup* mw, const char* content) {
    bool ret = false;
    if (tm_page_is_inited(mw)) {
        mw->content = content;
        ret = true;
    }
    return ret;
}
bool tm_msg_popup_slide_up_txt(tm_msg_popup* mw) // 返回值表示能否成功滑动
{
    tm_ui* ui = mw->page.ui;
    bool ret = false;
    if (mw->vars.autonewline_need_h + mw->str_start_y >
        MSG_WIN_H_MAX(ui) - 2 * MSG_WIN_FONT_MARGIN) {
        mw->str_start_y -= mw->str_move_step;
        ret = true;
    }
    return ret;
}
bool tm_msg_popup_slide_down_txt(tm_msg_popup* mw) {
    bool ret = false;
    if (mw->str_start_y < 0) {
        mw->str_start_y += mw->str_move_step;
        ret = true;
    }
    return ret;
}
