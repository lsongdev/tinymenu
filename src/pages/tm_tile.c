#include "core/tm_ui.h"
#include "pages/tm_tile.h"

//===================================================Title页面====================================================
// 磁贴宏参数,所有磁贴页面都使用同一套参数
#define TILE_ICON_IND_U DEFAULT_TILE_ICON_IND_U // 磁贴指示器与磁贴的上边距
#define TILE_ICON_IND_D DEFAULT_TILE_ICON_IND_D // 磁贴指示器与磁贴的下边距
#define TILE_ICON_IND_L DEFAULT_TILE_ICON_IND_L // 磁贴指示器与磁贴的左边距
#define TILE_ICON_IND_R DEFAULT_TILE_ICON_IND_R // 磁贴指示器与磁贴的右边距
#define TILE_ICON_IND_SL DEFAULT_TILE_ICON_IND_SL                         // 磁贴指示器边长
#define TILE_ASSERT_IND_H                                                                          \
    (TILE_ICON_IND_R + TILE_ICON_IND_L > DEFAULT_TILE_ICON_S) // 检查指示器水平大小

// The items (icon and name) are centred on the screen.
static int16_t block_h(const tm_tile_page* t) { return tm_icon_item_h(t->icon_h, t->name_mode != TM_ICON_NAME_NONE); }
static int16_t box_top(const tm_tile_page* t) { return (t->page.ui->fb.h - block_h(t)) / 2; } // 选中框上沿
// Left edge of the box of an item whose icon would start at icon_x.
#define BOX_X(icon_x) ((icon_x) - TILE_ICON_IND_L)
// Room for a name: up to the neighbours' names, or the screen when only the selected has one.
// Rooms never overlap, so a long name is cut (or scrolls when selected).
#define NAME_W(t) ((t)->name_mode == TM_ICON_NAME_ALL ? (t)->vars.pitch - 2 : (t)->page.ui->fb.w)
// The selected item's name scrolls when it does not fit; the others are cut.
#define NAME_SS(t, i) tm_icon_menu_name_slide((tm_icon_menu*)(t), (i))
#define TILE_ICON_S (self->vars.pitch) // 图标间距(中心到中心)

//**********************每个以Page为基类的类都有对应的AnimInit、show、React方法
//--------Title相关基本方法
void tm_tile_page_in_init(tm_page_addr page_addr) {
    tm_tile_page* self = (tm_tile_page*)page_addr;
    self->vars.pitch = DEFAULT_TILE_ICON_S + self->icon_w; // 固定间距，放不下的名称截断或滚动
    self->vars.iconX.pos_cur = 0;
    self->vars.iconX.pos_tgt = TILE_ICON_S;
    self->vars.iconY.pos_cur = -(self->icon_h / 2) - TILE_ICON_IND_U; // 选中框上沿
    self->vars.iconY.pos_tgt = box_top(self);
    tm_icon_menu_name_reset((tm_icon_menu*)self);
}
bool tm_tile_page_in(tm_page_addr page_addr) {
    tm_tile_page* self = (tm_tile_page*)page_addr;
    tm_ui* ui = self->page.ui;
    bool ret = false; // 表示动画是否结束，返回True UI会切换状态
    int16_t temp = 0; // 用于存放临时的icon的x坐标
    // 计算动画参数
    tm_animation(&self->vars.iconX, tm_ui_anim_ms(ui, TM_ANI_TILE), ui->time,
                     &(ui->anim_is_finish)); // 图标x坐标
    tm_animation(&self->vars.iconY, tm_ui_anim_ms(ui, TM_ANI_TILE), ui->time,
                     &(ui->anim_is_finish)); // 图标y坐标
    for (uint8_t i = 0; i < self->item_num; i++) {
        if (ui->params.ufd_param[TM_UNFOLD_TILE]) // 从第一个选项开始展开，最终保持选中在中间
            temp = (((ui)->fb.w - self->icon_w) >> 1) + i * self->vars.iconX.pos_cur -
                   TILE_ICON_S * self->select_item;
        else // 保证选中的选项在中间，向两侧展开
            temp = (((ui)->fb.w - self->icon_w) >> 1) +
                   (i - self->select_item) * self->vars.iconX.pos_cur;
        tm_icon_item_draw(&(ui->w_all), BOX_X(temp), (int16_t)(self->vars.iconY.pos_cur),
                          &self->option_array[i], self->icon_w, self->icon_h,
                          tm_icon_menu_shows_name((tm_icon_menu*)self, i),
                          NAME_W(self), NAME_SS(self, i), ui->time);
    }
    if (self->vars.iconX.pos_cur == self->vars.iconX.pos_tgt) {
        ret = true;
        self->vars.iconX.pos_cur = self->vars.iconX.pos_tgt =
            -1 * self->select_item * TILE_ICON_S;
    }
    return ret;
}
void tm_tile_page_show(tm_page_addr page_addr) {
    tm_tile_page* self = (tm_tile_page*)page_addr;
    tm_ui* ui = self->page.ui;
    // 计算动画参数
    tm_animation(&self->vars.iconX, tm_ui_anim_ms(ui, TM_ANI_TILE), ui->time,
                     &(ui->anim_is_finish)); // 图标x坐标
    for (uint8_t i = 0; i < self->item_num; i++) // 过度动画完成后一般选择时的切换动画
        tm_icon_item_draw(&(ui->w_all),
                          BOX_X(((ui)->fb.w - self->icon_w) / 2 + (int16_t)(self->vars.iconX.pos_cur) +
                                i * TILE_ICON_S),
                          box_top(self), &self->option_array[i], self->icon_w, self->icon_h,
                          tm_icon_menu_shows_name((tm_icon_menu*)self, i), NAME_W(self),
                          NAME_SS(self, i), ui->time);
    tm_icon_menu_name_step((tm_icon_menu*)self);
}
void tm_tile_page_indicator_ctrl(tm_page_addr page_addr) {
    tm_tile_page* self = (tm_tile_page*)page_addr;
    tm_ui* ui = self->page.ui;
    // indicator
    ui->indicator.x.pos_tgt = BOX_X(((ui)->fb.w / 2) - self->icon_w / 2);
    ui->indicator.y.pos_tgt = box_top(self);
    ui->indicator.w.pos_tgt = tm_icon_item_w(self->icon_w);
    ui->indicator.h.pos_tgt = tm_icon_item_h(self->icon_h, false); // 只框图标，不包括名称
    tm_ui_draw_highlight(ui, 3);
}
static void tile_prev(tm_tile_page* tp);
static void tile_next(tm_tile_page* tp);

bool tm_tile_page_react(tm_page_addr page_addr) {
    tm_tile_page* self = (tm_tile_page*)page_addr;
    tm_ui* ui = self->page.ui;
    bool ret = false; // 返回true时会触发UI切换状态
    tm_msg msg = tm_msg_que_read(&(ui->msg_queue));
    // 只要title页面有输入msg，滚动就重置(防止破坏动画)(可以只判断up/down,click和return在INparaInit中重置就可以)
    if (self->page.auto_input) { // 使能自动处理msg的话
        switch (msg) {
        case TM_MSG_PREV:
        case TM_MSG_LEFT:
        case TM_MSG_DOWN: // 横排：左右为主；上下没有方向含义，上向前(向右)，下向后
            tile_prev(self);
            break;
        case TM_MSG_NEXT:
        case TM_MSG_RIGHT:
        case TM_MSG_UP:
            tile_next(self);
            break;
        case TM_MSG_CLICK:
            break;
        case TM_MSG_RETURN:
            tm_page_return(page_addr);
            ret = true; // 通知UI切换状态
            break;
        default:
            break;
        }
        if (msg != TM_MSG_NONE && NULL != self->page.cb) // 任何输入的有效信息都会调用回调函数
            self->page.cb(
                &(self->page),
                msg); // 自动处理时,会忽略回调函数返回值(title内只有msg_return输入时才会返回)
    } else {
        if (msg != TM_MSG_NONE && NULL != self->page.cb) {
            ret = self->page.cb(&(self->page), msg); // 使用者在回调函数返回true,表示退出这个页面
            if (ret == true)
                tm_page_return(page_addr);
        }
    }
    return ret;
}
static const tm_page_methods tm_tile_page_methods = {
    .in = tm_tile_page_in,
    .in_para_init = tm_tile_page_in_init,
    .show = tm_tile_page_show,
    .react = tm_tile_page_react,
    .indicator_ctrl = tm_tile_page_indicator_ctrl,
    .scrollbar_ctrl = tm_func_do_nothing,
};

void tm_tile_page_init(
    tm_tile_page* tile,
    tm_ui* ui,  // 磁贴页面对象
    uint8_t item_num,       // 选项个数，与选项数组的大小一致(最多255个)
    tm_option* option_array,   // 整个页面的选项数组(数组大小需与item_num一致)
    tm_page_cb call_back) // 回调函数，参数为确认选中项index（1-256）0表示未确认哪个选项
{
    tile->page.page_type = TM_PAGE_TILE;
    tm_page_init((tm_page_addr)tile, ui, call_back);
    memset(&tile->vars, 0, sizeof(tile->vars));
    tile->page.methods = &tm_tile_page_methods; // 关联处理函数(方法)
    tm_icon_menu_set_items((tm_icon_menu*)tile, item_num, option_array); // 默认 30x30，显示名称
    if (block_h(tile) > (ui)->fb.h)
        TM_LOG_W("the icons and names of the tile page are taller than the screen");
    tm_tile_page_in_init(
        (tm_page_addr)tile); // 做一次进入参数初始化，因为默认UI中没有设置类变量的值
}
// Title提供的接口函数
static void tile_prev(tm_tile_page* tp) {
    tm_ui* ui = tp->page.ui;
    // 动画参数重置
    if (tp->select_item > 0) { // 不是第一个选项
        tp->select_item--;
        tp->vars.iconX.pos_tgt += tp->vars.pitch;
        ui->indicator.x.pos_cur +=
            (tp->icon_w >> 1); // 图标移动时从上一个中心移动到这一次的边角
    } else {                    // 是第一个选项
        if (ui->params.loop_param[TM_LOOP_TILE]) // 开启循环的话
        {
            tp->select_item = tp->item_num - 1;
            tp->vars.iconX.pos_tgt = -1 * tp->vars.pitch * (tp->item_num - 1);
            ui->indicator.x.pos_cur -= (tp->icon_w >> 1);
        }
    }
}
static void tile_next(tm_tile_page* tp) {
    tm_ui* ui = tp->page.ui;
    // 动画参数重置
    if (tp->select_item < (tp->item_num - 1)) { // 不是最后一个选项
        tp->select_item++;
        tp->vars.iconX.pos_tgt -= tp->vars.pitch;
        ui->indicator.x.pos_cur -= (tp->icon_w >> 1);
    } else { // 是最后一个选项
        if (ui->params.loop_param[TM_LOOP_TILE]) {
            tp->select_item = 0;
            tp->vars.iconX.pos_tgt = 0;
            ui->indicator.x.pos_cur += (tp->icon_w >> 1);
        }
    }
}

bool tm_tile_page_move(tm_tile_page* tp, int16_t delta) {
    const uint8_t from = tp->select_item;
    for (; delta < 0; delta++) tile_prev(tp);
    for (; delta > 0; delta--) tile_next(tp);
    return tp->select_item != from;
}

void tm_tile_page_select(tm_tile_page* tp, uint8_t index) {
    if (index < tp->item_num)
        tm_tile_page_move(tp, (int16_t)index - tp->select_item); // 不会绕回：始终在范围内
}
