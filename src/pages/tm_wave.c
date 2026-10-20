#include "core/tm_ui.h"
#include "pages/tm_wave.h"
#include <math.h>

//===================================================Wave页面====================================================
// 波形相关参数
#define WAVE_FONT DEFAULT_WAVE_FONT                                          // 波形字体
#define WAVE_BOX_L_S DEFAULT_WAVE_BOX_L_S                                    // 波形边框左边距
#define WAVE_BOX_R_S DEFAULT_WAVE_BOX_R_S                                    // 波形边框右边距
#define WAVE_BOX_U_S DEFAULT_WAVE_BOX_U_S                                    // 波形边框上边距
#define WAVE_BOX_D_S DEFAULT_WAVE_BOX_D_S                                    // 波形边框下边距
#define WAVE_BOX_W(ui) ((ui)->fb.w - DEFAULT_WAVE_Y_AXIS_LABEL_WIDTH - 1) // 波形边框宽度
#define WAVE_BOX_H(ui)                                                                                 \
    ((ui)->fb.h - GET_FNOT_H(WAVE_FONT) - DEFAULT_WAVE_BOX_D_S -                           \
     DEFAULT_WAVE_TEXT_D_S)                          // 波形边框高度
#define WAVE_STOP_SIG_HW (GET_FNOT_H(WAVE_FONT) - 1) // 波形停止标志高度/宽度，为方形保持与字体同高
#define WAVE_Y_AXIS_LABEL_WIDTH DEFAULT_WAVE_Y_AXIS_LABEL_WIDTH // y轴标签的宽度
#define WAVE_TEXT_L_S DEFAULT_WAVE_D_TXT_VAL_S                  // 波形文本左边距
#define WAVE_TEXT_R_S DEFAULT_WAVE_D_TXT_VAL_S                  // 波形文本右边距
#define WAVE_TEXT_D_S DEFAULT_WAVE_TEXT_D_S                     // 波形边框下边距
#define WAVE_TEXT_W DEFAULT_WAVE_TEXT_W                         // 文字背景框宽度
#define WAVE_SHOW_WIDTH(ui) (WAVE_BOX_W(ui) - 2)                         // 波形展示的宽度

//--------WavePage相关方法和函数
static int16_t _roundToNearestTen(int16_t value) {
    int16_t abs_val = abs(value);
    int16_t result;
    if (abs_val % 10 <= 5)
        result = (abs_val / 10) * 10;
    else
        result = ((abs_val / 10) + 1) * 10;
    return value < 0 ? -result : result;
}

static bool index_is_in_show_range(uint16_t idx, uint16_t idx_show_head,
                                       uint16_t idx_show_tail) {
    if (idx_show_head < idx_show_tail) // 队列没有回绕
        return (idx >= idx_show_head && idx < idx_show_tail);
    else if (idx_show_head > idx_show_tail) // 队列回绕
        return (idx >= idx_show_head || idx < idx_show_tail);
    else
        return false; // 队列空的时候，也不在范围内
}

static void wave_update_range(tm_wave_data* wd) {
    if (wd->fixed_min != wd->fixed_max) { // 固定范围：不随数据变化
        wd->range_min = MIN(wd->fixed_min, wd->fixed_max);
        wd->range_max = MAX(wd->fixed_min, wd->fixed_max);
        return;
    }
    bool maxFlag = false; // 最大值改变标志
    bool minFlag = false; // 最小值改变标志
    uint16_t idx_show_head = wd->idx_show_head, idx_show_tail = wd->idx_show_tail;
    if (!index_is_in_show_range(wd->idx_show_max, idx_show_head,
                                    idx_show_tail)) { // 如果最大值已经移出展示范围
        int16_t max =
            -32768; // 从新计算一次最大值(用最小值作为比较初值，省去了第一次判断，还是很巧妙的)
        for (uint16_t i = idx_show_head; i != idx_show_tail; i++, i %= wd->depth)
            if (wd->data[i] >= max) {
                max = wd->data[i];
                wd->idx_show_max = i;
            }
        maxFlag = true;
    }
    if (!index_is_in_show_range(wd->idx_show_min, idx_show_head,
                                    idx_show_tail)) { // 如果最小值已经移出展示范围
        int16_t min = 32767;                          // 从新计算一次最小值
        for (uint16_t i = idx_show_head; i != idx_show_tail; i++, i %= wd->depth)
            if (wd->data[i] <= min) {
                min = wd->data[i];
                wd->idx_show_min = i;
            }
        minFlag = true;
    }
    // 这个函数只能用于更新左右移动一步的情况，因为下面只有判断队列两边进入的一个值
    if (wd->data[idx_show_head] >=
        wd->data[wd->idx_show_max]) {     // 新加入的数据大于最大值，更新最大值
        wd->idx_show_max = idx_show_head; // 新加入的数据大于最大值，更新最大值
        maxFlag = true;
    }
    if (wd->data[idx_show_head] <=
        wd->data[wd->idx_show_min]) { // 新加入的数据小于最大值，更新最大值
        wd->idx_show_min = idx_show_head;
        minFlag = true;
    }
    if (wd->data[(idx_show_tail + wd->depth - 1) % wd->depth] >=
        wd->data[wd->idx_show_max]) { // 新加入的数据大于最大值，更新最大值
        wd->idx_show_max = (idx_show_tail + wd->depth - 1) % wd->depth;
        maxFlag = true;
    }
    if (wd->data[(idx_show_tail + wd->depth - 1) % wd->depth] <=
        wd->data[wd->idx_show_min]) { // 新加入的数据小于最大值，更新最大值
        wd->idx_show_min = (idx_show_tail + wd->depth - 1) % wd->depth;
        minFlag = true;
    }
    if (maxFlag) // 最大值改变
        wd->range_max = _roundToNearestTen(wd->data[wd->idx_show_max] + 10);
    if (minFlag) // 最小值改变
        wd->range_min = _roundToNearestTen(wd->data[wd->idx_show_min] - 10);
}

static void wave_anim_reset(tm_wave_page* self) { // 这个函数抽象出来仅仅是因为用得多
    tm_ui* ui = self->page.ui;
    self->vars.text_y.pos_cur = (ui)->fb.h;
    self->vars.y_axis_val_x.pos_cur = (ui)->fb.w; // 下面两个可以放全局变量里初始化其实
    self->vars.text_y.pos_tgt = WAVE_BOX_U_S + WAVE_BOX_H(ui) + WAVE_BOX_D_S;
    self->vars.y_axis_val_x.pos_tgt = WAVE_BOX_L_S + WAVE_BOX_W(ui) + 1;
    tm_slide_str_reset(&(self->vars.title_ss));
    tm_slide_str_reset(&(self->vars.val_ss));
    tm_slide_str_reset(&(self->vars.max_val_ss));
    tm_slide_str_reset(&(self->vars.mid_val_ss));
    tm_slide_str_reset(&(self->vars.min_val_ss));
}

void tm_wave_page_in_init(tm_page_addr page_addr) {
    tm_wave_page* self = (tm_wave_page*)page_addr;
    tm_ui* ui = self->page.ui;
    UNUSED_PARAMETER(page_addr);
    self->vars.title_ss.slide_mode = ui->params.slide_mode;
    self->vars.val_ss.slide_mode = ui->params.slide_mode;
    self->vars.max_val_ss.slide_mode = ui->params.slide_mode;
    self->vars.mid_val_ss.slide_mode = ui->params.slide_mode;
    self->vars.min_val_ss.slide_mode = ui->params.slide_mode;
    wave_anim_reset(self);
}

// bool tm_wave_page_in(tm_page_addr page_addr)
// {
// //文字和y轴的动画可以在过度动画做，把具体绘制部分代码分离成函数调用即可，那部分代码有点多，因为那段动画挺短的，In动画不做也行
//   //不做In动画(这个函数没啥用)直接使用默认返回true函数初始化类方法就可以了。
//     tm_wave_page *wp = (tm_wave_page*)page_addr;
//     bool ret = true; // 默认动画完成，返回true进入下一个状态
//     return ret;
// }
// Y-axis labels drop decimals until they fit the label column (-10.1 -> -10); a label
// that is still too wide scrolls like any other slide string.
static char* wave_axis_label(char* buf, size_t len, int32_t val, tm_decimals decimals) {
    for (int d = decimals;; d--) {
        tm_format_general(buf, len, val, (tm_decimals)d);
        if (d == 0 || tm_text_width(buf, WAVE_FONT) <= WAVE_Y_AXIS_LABEL_WIDTH)
            return buf;
        val = (val + (val < 0 ? -5 : 5)) / 10;
    }
}

void tm_wave_page_show(tm_page_addr page_addr) {
    tm_wave_page* self = (tm_wave_page*)page_addr;
    tm_ui* ui = self->page.ui;
    char val_text[16], max_text[16], mid_text[16], min_text[16]; // 只在本次绘制中使用
    uint16_t wave_x = 0;  // 波形点的x坐标,从1开始，因为0是边框
    int16_t data_val = 0; // 波形点的数据
    tm_wave_data* wd = &(self->waves[self->selected]);
    // 绘制波形(绘制选中波形数据)
    for (uint16_t j = wd->idx_show_head; j != wd->idx_show_tail; j++, j %= wd->depth) {
        wave_x++; // 取点和像素计算
        data_val = wd->data[j];
        // 这儿有必要做这个限制嘛？？？？因为每次更新进去，我们不应该保证wp保存的最大最小值是所有波形的最大最小值
        data_val = data_val < wd->range_min ? wd->range_min : data_val; // 限制波形数据范围
        data_val = data_val > wd->range_max ? wd->range_max : data_val;
        // 范围两端落在框内 1 像素处，最大值和最小值(例如 GPIO 的 1 和 0)都看得见
        self->vars.cur_y =
            WAVE_BOX_H(ui) + WAVE_BOX_U_S - 2 -
            (data_val - wd->range_min) * (WAVE_BOX_H(ui) - 3) / (wd->range_max - wd->range_min);
        if (j == wd->idx_show_head)
            self->vars.last_y = self->vars.cur_y;
        // 绘制波形
        switch (wd->type) {
        case TM_WAVE_SOLID: // 实线
            if (self->vars.cur_y > (self->vars.last_y + 1))
                tm_canvas_draw_line_v(&(ui->w_all), WAVE_BOX_L_S + wave_x,
                                        self->vars.last_y + 1, self->vars.cur_y);
            else if (self->vars.cur_y < (self->vars.last_y - 1))
                tm_canvas_draw_line_v(&(ui->w_all), WAVE_BOX_L_S + wave_x,
                                        self->vars.last_y - 1, self->vars.cur_y);
            else
                tm_canvas_draw_point(&(ui->w_all), WAVE_BOX_L_S + wave_x,
                                       self->vars.cur_y);
            break;
        case TM_WAVE_DASH:                                                 // 虚线
            if (wave_x % 2 == 0) {                                          // 间隔一个点画一个点
                if (self->vars.cur_y > (self->vars.last_y + 1)) // data2
                    tm_canvas_draw_line_v(&(ui->w_all), WAVE_BOX_L_S + wave_x,
                                            self->vars.last_y + 1, self->vars.cur_y);
                else if (self->vars.cur_y < (self->vars.last_y - 1))
                    tm_canvas_draw_line_v(&(ui->w_all), WAVE_BOX_L_S + wave_x,
                                            self->vars.last_y - 1, self->vars.cur_y);
                else
                    tm_canvas_draw_point(&(ui->w_all), WAVE_BOX_L_S + wave_x,
                                           self->vars.cur_y);
            }
            break;
        case TM_WAVE_STEP: // 阶梯：变化时整段竖线连到新电平，不变时画水平线
            if (self->vars.cur_y != self->vars.last_y)
                tm_canvas_draw_line_v(&(ui->w_all), WAVE_BOX_L_S + wave_x,
                                      MIN(self->vars.last_y, self->vars.cur_y),
                                      MAX(self->vars.last_y, self->vars.cur_y));
            else
                tm_canvas_draw_point(&(ui->w_all), WAVE_BOX_L_S + wave_x, self->vars.cur_y);
            break;
        case TM_WAVE_FILL: // 填充波形
            tm_canvas_draw_line_v(&(ui->w_all), WAVE_BOX_L_S + wave_x,
                                    WAVE_BOX_U_S + WAVE_BOX_H(ui) - 1, self->vars.cur_y);
            break;
        default:
            break;
        }
        self->vars.last_y = self->vars.cur_y;
    }
    // 绘制start/stop标志
    if (false == wd->stopped)
        tm_canvas_draw_rbox(&(ui->w_all), 1, WAVE_BOX_U_S + WAVE_BOX_H(ui) + WAVE_BOX_D_S,
                              WAVE_STOP_SIG_HW, WAVE_STOP_SIG_HW,
                              UINT_DIVISION_CELL(WAVE_STOP_SIG_HW, 3));
    else
        tm_canvas_draw_rbox(&(ui->w_all), 1, WAVE_BOX_U_S + WAVE_BOX_H(ui) + WAVE_BOX_D_S,
                              WAVE_STOP_SIG_HW, WAVE_STOP_SIG_HW, 0);
    tm_animation(&self->vars.text_y, tm_ui_anim_ms(ui, TM_ANI_WAVE), ui->time,
                     &(ui->anim_is_finish)); // 文字y坐标
    tm_animation(&self->vars.y_axis_val_x, tm_ui_anim_ms(ui, TM_ANI_WAVE),
                     ui->time, &(ui->anim_is_finish)); // 文字x坐标
    // 绘制Title字符串
    self->vars.title_ss.canvas.start_x = WAVE_STOP_SIG_HW + 2 + WAVE_TEXT_L_S;
    self->vars.title_ss.canvas.start_y = self->vars.text_y.pos_cur;
    self->vars.title_ss.canvas.h = GET_FNOT_H(WAVE_FONT);
    self->vars.title_ss.canvas.w = WAVE_TEXT_W;
    self->vars.title_ss.str = wd->text;
    tm_slide_str_draw(&(self->vars.title_ss), 0, WAVE_FONT, ui->time);
    // 绘制下面的cur值
    self->vars.val_ss.str =
        tm_format_general(val_text, sizeof(val_text),
                          wd->data[(wd->idx_show_tail + wd->depth - 1) % wd->depth], wd->decimals);
    self->vars.val_ss.canvas.w =
        (ui)->fb.w - WAVE_TEXT_L_S - WAVE_TEXT_R_S - WAVE_TEXT_W - WAVE_STOP_SIG_HW - 2;
    self->vars.val_ss.canvas.h = GET_FNOT_H(WAVE_FONT);
    self->vars.val_ss.canvas.start_x =
        WAVE_TEXT_W + WAVE_STOP_SIG_HW + 2 + WAVE_TEXT_L_S + WAVE_TEXT_R_S;
    self->vars.val_ss.canvas.start_y = self->vars.text_y.pos_cur;
    tm_slide_str_draw(&(self->vars.val_ss), 0, WAVE_FONT, ui->time);
    // 绘制侧边的最大值和最小值和中间值
    self->vars.max_val_ss.canvas.start_x = self->vars.y_axis_val_x.pos_cur;
    self->vars.max_val_ss.canvas.start_y = WAVE_BOX_U_S;
    self->vars.max_val_ss.canvas.h = GET_FNOT_H(WAVE_FONT);
    self->vars.max_val_ss.canvas.w = WAVE_Y_AXIS_LABEL_WIDTH;
    self->vars.max_val_ss.str = wave_axis_label(max_text, sizeof(max_text), wd->range_max, wd->decimals);
    tm_slide_str_draw(&(self->vars.max_val_ss), 0, WAVE_FONT, ui->time);
    self->vars.mid_val_ss.canvas = self->vars.max_val_ss.canvas;
    self->vars.mid_val_ss.canvas.start_y =
        WAVE_BOX_U_S + (WAVE_BOX_H(ui) >> 1) - (WAVE_FONT.Height >> 1);
    self->vars.mid_val_ss.str = wave_axis_label(mid_text, sizeof(mid_text), (wd->range_max + wd->range_min) / 2, wd->decimals);
    if (wd->range_max - wd->range_min >= 2) // 范围太小(如 0~1)时中值和端点相同，不画
        tm_slide_str_draw(&(self->vars.mid_val_ss), 0, WAVE_FONT, ui->time);
    self->vars.min_val_ss.canvas = self->vars.max_val_ss.canvas;
    self->vars.min_val_ss.canvas.start_y = WAVE_BOX_U_S + WAVE_BOX_H(ui) - WAVE_FONT.Height;
    self->vars.min_val_ss.str = wave_axis_label(min_text, sizeof(min_text), wd->range_min, wd->decimals);
    tm_slide_str_draw(&(self->vars.min_val_ss), 0, WAVE_FONT, ui->time);
    if (ui->anim_is_finish) { // anim结束，且单次滚动没有完成就使能滚动
        if (!(self->vars.title_ss.slide_is_finish))
            self->vars.title_ss.slide_enable = true;
        if (!(self->vars.val_ss.slide_is_finish))
            self->vars.val_ss.slide_enable = true;
        if (!(self->vars.max_val_ss.slide_is_finish))
            self->vars.max_val_ss.slide_enable = true;
        if (!(self->vars.mid_val_ss.slide_is_finish))
            self->vars.mid_val_ss.slide_enable = true;
        if (!(self->vars.min_val_ss.slide_is_finish))
            self->vars.min_val_ss.slide_enable = true;
    }
    // 纵轴刻度
    uint16_t axis_tick_x = ui->indicator.x.pos_cur + ui->indicator.w.pos_cur;
    tm_canvas_draw_point(&(ui->w_all), axis_tick_x, ui->indicator.y.pos_cur);
    tm_canvas_draw_point(&(ui->w_all), axis_tick_x,
                           ui->indicator.y.pos_cur +
                               UINT_DIVISION_ROUND(ui->indicator.h.pos_cur, 2) - 1);
    tm_canvas_draw_point(&(ui->w_all), axis_tick_x,
                           ui->indicator.y.pos_cur + ui->indicator.h.pos_cur - 1);
    ui->slide_is_finish =
        !(self->vars.title_ss.slide_enable) && !(self->vars.val_ss.slide_enable) &&
        !(self->vars.max_val_ss.slide_enable) &&
        !(self->vars.mid_val_ss.slide_enable) &&
        !(self->vars.min_val_ss.slide_enable) &&
        wd->stopped; // 记录这个页面中所有slide动画的结果&&(同时页面必须是停止状态，不然还是通过slide_is_finish这个标志位控制is_motion_less这个标志位)
}
bool tm_wave_page_react(tm_page_addr page_addr) {
    tm_wave_page* self = (tm_wave_page*)page_addr;
    tm_ui* ui = self->page.ui;
    bool ret = false;
    tm_msg msg = tm_msg_que_read(&(ui->msg_queue));
    tm_wave_data* wd = &(self->waves[self->selected]);
    if (self->page.auto_input) { // 自动处理msg的话
        switch (msg) {
        case TM_MSG_PREV:
        case TM_MSG_DOWN: // 横向：左右为主；上下没有方向含义，上向前(向右)，下向后
        case TM_MSG_LEFT:
            if (false == wd->stopped) // 运动状态下,上左切换波形
                tm_wave_page_move(self, -1);
            else // 停止状态下移动波形查看
                tm_wave_page_scroll(self, self->selected, -1);
            break;
        case TM_MSG_NEXT:
        case TM_MSG_UP:
        case TM_MSG_RIGHT:
            if (false == wd->stopped)
                tm_wave_page_move(self, 1);
            else
                tm_wave_page_scroll(self, self->selected, 1);
            break;
        case TM_MSG_CLICK:
            tm_wave_page_stop(self, self->selected, !wd->stopped);
            break;
        case TM_MSG_RETURN:
            tm_page_return(page_addr);
            ret = true; // 通知UI切换状态
            break;
        default:
            break;
        }
        if (TM_MSG_NONE != msg && NULL != self->page.cb)
            self->page.cb(&(self->page), msg); // 任何输入的有效信息都会调用回调函数
    } else {
        if (TM_MSG_NONE != msg && NULL != self->page.cb) { // 任何输入的有效信息都会调用回调函数
            ret = self->page.cb(&(self->page), msg); // 使用者在回调函数返回true,表示退出这个页面
            if (ret == true)
                tm_page_return(page_addr);
        }
    }
    return ret;
}
void tm_wave_page_indicator_ctrl(tm_page_addr page_addr) {
    tm_wave_page* self = (tm_wave_page*)page_addr;
    tm_ui* ui = self->page.ui;
    UNUSED_PARAMETER(page_addr);
    // indicator也是波形外框
    ui->indicator.x.pos_tgt = WAVE_BOX_L_S;
    ui->indicator.y.pos_tgt = WAVE_BOX_U_S;
    ui->indicator.w.pos_tgt = WAVE_BOX_W(ui);
    ui->indicator.h.pos_tgt = WAVE_BOX_H(ui);
    tm_canvas_draw_rbox_empty(&(ui->w_all), ui->indicator.x.pos_cur,
                               ui->indicator.y.pos_cur, ui->indicator.w.pos_cur,
                               ui->indicator.h.pos_cur, 0);
}

static const tm_page_methods tm_wave_page_methods = {
    .in = tm_func_do_nothing_ret_true,
    .in_para_init = tm_wave_page_in_init,
    .show = tm_wave_page_show,
    .react = tm_wave_page_react,
    .indicator_ctrl = tm_wave_page_indicator_ctrl,
    .scrollbar_ctrl = tm_func_do_nothing,
};

void tm_wave_page_init(tm_wave_page* wp,
    tm_ui* ui,              // 波形页面对象
                         uint8_t wave_num,          // 波形数量
                         tm_wave_data* waves, // 波形数据数组
                         tm_page_cb call_back)    // 回调函数
{
    wp->page.page_type = TM_PAGE_WAVE;
    tm_page_init((tm_page_addr)wp, ui, call_back);
    memset(&wp->vars, 0, sizeof(wp->vars));
    wp->vars.title_ss.slide_mode = ui->params.slide_mode;
    wp->vars.val_ss.slide_mode = ui->params.slide_mode;
    wp->vars.max_val_ss.slide_mode = ui->params.slide_mode;
    wp->vars.min_val_ss.slide_mode = ui->params.slide_mode;
    wp->vars.mid_val_ss.slide_mode = ui->params.slide_mode;
    wp->vars.title_ss.canvas.fb = &ui->fb;
    wp->vars.val_ss.canvas.fb = &ui->fb;
    wp->vars.max_val_ss.canvas.fb = &ui->fb;
    wp->vars.min_val_ss.canvas.fb = &ui->fb;
    wp->vars.mid_val_ss.canvas.fb = &ui->fb;
    wp->wave_count = wave_num;
    wp->waves = waves;
    wp->selected = 0;
    wp->page.methods = &tm_wave_page_methods;
    for (uint8_t i = 0; i < wave_num; i++) {
        wp->waves[i].idx_head = 0;
        wp->waves[i].idx_tail = 0;
        wp->waves[i].idx_show_head = 0;
        wp->waves[i].idx_show_tail = 0;
        wp->waves[i].idx_show_max = 0;
        wp->waves[i].idx_show_min = 0;
        wp->waves[i].range_max = -32768;
        wp->waves[i].range_min = 32767;
        wp->waves[i].stopped = false; // 默认是运动的
        if (wp->waves[i].fixed_min != wp->waves[i].fixed_max)
            wave_update_range(&wp->waves[i]); // 固定范围一开始就生效
    }
}

// wave页面给使用者的接口函数
void tm_wave_page_push(tm_wave_page* wp, uint8_t wave_num, int16_t new_data) {
    tm_ui* ui = wp->page.ui; // 更新波形数据
    if (wave_num >= wp->wave_count) {
        TM_LOG_E("Valid wave_num, it is too large");
        return;
    }
    tm_wave_data* wave_data = &(wp->waves[wave_num]);
    if (false == wave_data->stopped) // 波形运动时才能更新数据
    {
        if ((tm_page*)wp == tm_ui_current_page(ui) &&
            wave_num == wp->selected) // 往页面选中项页面存波形数据才有必要刷新屏幕
            ui->is_motionless = false; // 当有波形数据进入时，启用刷新(如果使用软件刷新的话)
        wave_data->data[wave_data->idx_tail] =
            new_data; // 放入新数据(先赋值再移动index,有利于其他从head到tail的数据循环，tail指向数据保持空)
        wave_data->idx_tail++;
        wave_data->idx_tail %= (wave_data->depth);            // 尾指针后移
        wave_data->idx_show_tail = wave_data->idx_tail; // 更新显示范围
        if (((wave_data->idx_show_tail + wave_data->depth - wave_data->idx_show_head) % wave_data->depth) >
            WAVE_SHOW_WIDTH(ui)) { // 如果显示范围扩大了
            wave_data->idx_show_head++;
            wave_data->idx_show_head %= (wave_data->depth); // 整体后移
        }
        if (wave_data->idx_tail ==
            wave_data->idx_head) { // 队列满了,直接覆盖(注意尾指针已经后移过了)
            wave_data->idx_head++;
            wave_data->idx_head %= (wave_data->depth);
        }
        wave_update_range(wave_data); // 更新数据范围
    }
}

void tm_wave_page_stop(tm_wave_page* wp, uint8_t wave_num, bool stop) {
    if (wave_num >= wp->wave_count) {
        TM_LOG_E("Valid wave_num, it is too large");
        return;
    }
    tm_wave_data* wd = &(wp->waves[wave_num]);
    wd->stopped = stop;
    if (false == wd->stopped) { // 从true->false,停止后从新开始，需要把展示指针移到最新的数据处
        int32_t delta = wd->idx_tail - wd->idx_show_tail;
        if (delta < 0) {
            delta += wd->depth;
        }
        wd->idx_show_head += delta; // show tail移动多少，head移动多少
        wd->idx_show_head %= wd->depth;
        wd->idx_show_tail = wd->idx_tail;
        wd->idx_show_max = wd->idx_show_tail;
        wd->idx_show_min =
            wd->idx_show_tail; // 把min和max的指针强制置到show的范围外(因为上面向右移动了不止一个点)
        wave_update_range(wd); // 这样调用这个函数可以强制对所有数据进行一次轮询最大最小值
    }
}
bool tm_wave_page_move(tm_wave_page* wp, int16_t delta) {
    if (wp->wave_count < 2 || delta == 0)
        return false;
    const int16_t n = wp->wave_count;
    tm_wave_page_select(wp, (uint8_t)(((wp->selected + delta) % n + n) % n)); // 首尾相接
    return true;
}

void tm_wave_page_select(tm_wave_page* wp, uint8_t index) {
    if (index >= wp->wave_count)
        return;
    wave_anim_reset(wp);
    wp->selected = index;
}

bool tm_wave_page_can_scroll(tm_wave_page* wp, uint8_t wave_num, int8_t dir) {
    if (wave_num >= wp->wave_count) {
        TM_LOG_E("Valid wave_num, it is too large");
        return false;
    }
    const tm_wave_data* wd = &(wp->waves[wave_num]);
    if (!wd->stopped || dir == 0) // 只有波形停止的时候才能翻看
        return false;
    return dir < 0 ? wd->idx_show_head != wd->idx_head : wd->idx_show_tail != wd->idx_tail;
}

bool tm_wave_page_scroll(tm_wave_page* wp, uint8_t wave_num, int8_t dir) {
    if (!tm_wave_page_can_scroll(wp, wave_num, dir))
        return false;
    tm_wave_data* wd = &(wp->waves[wave_num]);
    const uint16_t step = dir < 0 ? wd->depth - 1 : 1; // 往前(更早)或往后(更新)移一个采样
    wd->idx_show_head = (wd->idx_show_head + step) % wd->depth;
    wd->idx_show_tail = (wd->idx_show_tail + step) % wd->depth;
    wave_update_range(wd);
    return true;
}
