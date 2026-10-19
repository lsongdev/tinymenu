#include "core/tm_ui.h"

// 空函数用于赋值一些页面不需要的方法，防止调用到NULL,这样调用时可以不用考虑NULL指针检查
void tm_func_do_nothing(tm_page_addr page_addr) {
    UNUSED_PARAMETER(page_addr);
}
bool tm_func_do_nothing_ret_true(tm_page_addr page_addr) {
    UNUSED_PARAMETER(page_addr);
    return true;
}
bool tm_func_do_nothing_ret_false(tm_page_addr page_addr) {
    UNUSED_PARAMETER(page_addr);
    return false;
}

// 全局窗口变量，所有的绘制都在这个窗口内进行

//===================================全局变量==================================
// 全局UI参数集合对象(同时初始化)，这个UI的相关参数都在这个集合中定义
const tm_ui_params tm_default_params = {
    .style =
        {
            .animate = true,
            .radius = 255,
            .highlight = TM_HIGHLIGHT_FILL,
            .highlight_width = TM_HIGHLIGHT_FULL_ROW,
            .transition = TM_TRANSITION_CASCADE,
        },
    .ani_param =
        {
            [TM_ANI_INDICATOR] = 150,  // 指示器动画速度
            [TM_ANI_TILE] = 100, // 磁贴动画速度
            [TM_ANI_LIST] = 100, // 列表动画速度
            [TM_ANI_WAVE] = 100, // 波形动画速度
            [TM_ANI_POPUP] = 100,  // 弹窗动画速度
            [TM_ANI_FADE] = 20,  // 页面渐变退出速度
            [TM_ANI_SLIDE] = 100, // 页面侧滑和列表依次滑入的速度(与列表、磁贴一致)
        },
    .ufd_param =
        {
            [TM_UNFOLD_TILE] = false, // 磁贴图标从头展开开关
            [TM_UNFOLD_LIST] = false, // 菜单列表从头展开开关
        },
    .loop_param =
        {
            [TM_LOOP_TILE] = true,     // 磁贴图标循环模式开关
            [TM_LOOP_LIST] = true,     // 菜单列表循环模式开关
            [TM_LOOP_LIST_POPUP] = true, // 菜单弹窗循环模式开关
        },
    .slidestrstep_param =
        {
            [TM_SLIDE_STEP_TILE] = 2,      // 磁贴字符串滚动步进
            [TM_SLIDE_STEP_LIST_TEXT] = 1, // 列表文本滚动步进
            [TM_SLIDE_STEP_LIST_VALUE] = 1,  // 列表数值滚动步进
            [TM_SLIDE_STEP_WAVE_TEXT] = 1, // 波形文本滚动步进
            [TM_SLIDE_STEP_WAVE_VALUE] = 1,  // 波形数值滚动步进
            [TM_SLIDE_STEP_POPUP_TEXT] = 1,   // 弹窗文本滚动步进
            [TM_SLIDE_STEP_POPUP_VALUE] = 1,   // 弹窗数值滚动步进
        },
    .slidestrmode_param =
        {
            [TM_SLIDE_STEP_TILE] = TM_SLIDE_ONCE,      // 磁贴字符串滚动模式
            [TM_SLIDE_STEP_LIST_TEXT] = TM_SLIDE_ONCE, // 列表文本滚动模式
            [TM_SLIDE_STEP_LIST_VALUE] = TM_SLIDE_ONCE,  // 列表数值滚动模式
            [TM_SLIDE_STEP_WAVE_TEXT] = TM_SLIDE_ONCE, // 波形文本滚动模式
            [TM_SLIDE_STEP_WAVE_VALUE] = TM_SLIDE_ONCE,  // 波形数值滚动模式
            [TM_SLIDE_STEP_POPUP_TEXT] = TM_SLIDE_ONCE,   // 弹窗文本滚动模式
            [TM_SLIDE_STEP_POPUP_VALUE] = TM_SLIDE_ONCE,   // 弹窗数值滚动模式
        },
    .winbgblur_param =
        {
            [TM_POPUP_BLUR_MSG] = (tm_blur)2,  // 消息弹窗背景模糊程度
            [TM_POPUP_BLUR_CONFIRM] = (tm_blur)2, // 确认弹窗背景模糊程度
            [TM_POPUP_BLUR_VALUE] = (tm_blur)2,  // 数值弹窗背景模糊程度
            [TM_POPUP_BLUR_DIGIT] = (tm_blur)2, // 微调弹窗背景模糊程度
            [TM_POPUP_BLUR_TEXT_INPUT] = (tm_blur)2,
            [TM_POPUP_BLUR_PROGRESS] = (tm_blur)2,
            [TM_POPUP_BLUR_LIST] = (tm_blur)0, // 列表弹窗背景模糊程度
        },
};



char tm_option_mark(const tm_option* option) {
    static const char MARKS[] = {
        [TM_OPTION_PLAIN] = '-', [TM_OPTION_INT] = '=',    [TM_OPTION_FIXED] = '%',
        [TM_OPTION_TEXT] = '>',  [TM_OPTION_SWITCH] = '@', [TM_OPTION_CHECK] = '#',
    };
    if (option->mark)
        return option->mark;
    return (unsigned)option->kind < sizeof(MARKS) ? MARKS[option->kind] : '-';
}

float tm_option_float(tm_option* option) {
    switch (option->decimals) {
    case TM_DECIMALS_0:
        return option->val;
    case TM_DECIMALS_1:
        return option->val / 10.0f;
    case TM_DECIMALS_2:
        return option->val / 100.0f;
    case TM_DECIMALS_3:
        return option->val / 1000.0f;
    default:
        return option->val;
    }
}

uint16_t tm_ui_anim_ms(const tm_ui* ui, enum tm_ani_kind kind) {
    return ui->params.style.animate ? ui->params.ani_param[kind] : 0;
}

int16_t tm_ui_radius(const tm_ui* ui, int16_t r) {
    return MIN(r, (int16_t)ui->params.style.radius);
}

void tm_ui_set_preset(tm_ui* ui, tm_preset preset) {
    ui->params = tm_default_params;
    if (preset == TM_PRESET_CLASSIC) {
        ui->params.style = (tm_style){
            .animate = false,
            .radius = 0,
            .highlight = TM_HIGHLIGHT_FILL,
            .highlight_width = TM_HIGHLIGHT_FULL_ROW,
            .transition = TM_TRANSITION_FADE,
        };
        for (int i = 0; i < TM_POPUP_BLUR_COUNT; i++)
            ui->params.winbgblur_param[i] = TM_BLUR_0;
    }
    tm_ui_invalidate(ui);
}

void tm_ui_draw_highlight(tm_ui* ui, int16_t r) {
    const tm_indicator* ind = &ui->indicator;
    r = tm_ui_radius(ui, r);
    if (ui->params.style.highlight == TM_HIGHLIGHT_FRAME) {
        tm_canvas_draw_rbox_empty(&ui->w_all, ind->x.pos_cur, ind->y.pos_cur, ind->w.pos_cur,
                                  ind->h.pos_cur, r);
        return;
    }
    if (ui->params.style.highlight == TM_HIGHLIGHT_CORNERS) { // 角长最多 5 像素，小框按比例缩短
        const int16_t corner = MAX(MIN(MIN(ind->w.pos_cur, ind->h.pos_cur) / 3, 5), 1);
        tm_fb_set_color(&ui->fb, 2);
        tm_canvas_draw_box_right_angle(&ui->w_all, ind->x.pos_cur, ind->y.pos_cur, ind->w.pos_cur,
                                       ind->h.pos_cur, corner);
        tm_fb_set_color(&ui->fb, 1);
        return;
    }
    tm_fb_set_color(&ui->fb, 2); // 反色绘制，框内文字反白
    tm_canvas_draw_rbox(&ui->w_all, ind->x.pos_cur, ind->y.pos_cur, ind->w.pos_cur, ind->h.pos_cur, r);
    tm_fb_set_color(&ui->fb, 1);
}

//-------UI相关函数
// 设置当前正在操作的UI对象
void tm_ui_init(tm_ui* ui, uint8_t* buf, int16_t w, int16_t h, tm_flush_fn flush, void* user) {
    memset(ui, 0, sizeof(*ui));
    ui->params = tm_default_params;
    tm_fb_init(&ui->fb, buf, w, h);
    tm_fb_set_color(&ui->fb, 1);
    tm_fb_clear(&ui->fb);
    ui->flush = flush;
    ui->flush_user = user;
    ui->w_all = (tm_canvas){&ui->fb, 0, 0, w, h};
    ui->state = TM_STATE_IN; // 从没页面进入主页面
    ui->msg_queue.queue[0] = TM_MSG_NONE;
    ui->indicator.w.pos_cur = w;
    ui->indicator.h.pos_cur = h;
    ui->scrollBar.display = true;
    ui->ui_blur.blur_end = true;
    ui->time = 20;
    tm_ui_invalidate(ui);
}

void tm_ui_send(tm_ui* ui, tm_msg msg) {
    tm_msg_que_send(&ui->msg_queue, msg);
}

// FNV-1a of one page (8 rows) of the framebuffer.
static uint32_t tm_page_hash(const tm_fb* fb, int16_t page) {
    const uint8_t* p = fb->buf + (size_t)page * fb->w;
    uint32_t hash = 2166136261u;
    for (int16_t i = 0; i < fb->w; i++) hash = (hash ^ p[i]) * 16777619u;
    return hash;
}

// Hands the pages that changed since the last flush to the display driver.
static void tm_ui_flush_changed(tm_ui* ui) {
    const int16_t pages = tm_fb_pages(&ui->fb);
    int16_t first = pages, end = 0;
    for (int16_t page = 0; page < pages; page++) {
        if (page < TM_MAX_PAGES) {
            const uint32_t hash = tm_page_hash(&ui->fb, page);
            if (hash == ui->page_hash[page] && !ui->flush_all) continue;
            ui->page_hash[page] = hash;
        }
        if (page < first) first = page;
        end = page + 1;
    }
    ui->flush_all = false;
    if (first < end) ui->flush(&ui->fb, first, end, ui->flush_user);
}

void tm_ui_invalidate(tm_ui* ui) {
    ui->is_motionless = false;
    ui->flush_all = true;
}

tm_page* tm_ui_current_page(const tm_ui* ui) {
    return (tm_page*)ui->current_page;
}

static void tm_blur_para_init(tm_ui* ui, uint8_t blur_tgt, uint16_t blur_time) {
    ui->ui_blur.blur_tgt = blur_tgt;
    ui->ui_blur.blur_end = false;
    ui->ui_blur.blur_time = blur_time;
    ui->ui_blur.timer = 0;
}

/**
 * @brief UI画面渐变动画处理函数
 *
 * @param time 函数轮询间隔时间
 * @return 返回true表示任务完成，通知UI可以切换状态
 */
static bool tm_blur_proc(tm_ui* ui, uint16_t time) {
    if (!ui->params.style.animate) ui->ui_blur.blur_cur = ui->ui_blur.blur_tgt; // 直接到目标虚化程度
    if (false == ui->ui_blur
                     .blur_end) { // fade完成就不用fade了，可以作为一个开关，要开启fade的时候置false
        if (ui->ui_blur.timer <= ui->ui_blur.blur_time) { // 用于虚化过程的延时
            ui->ui_blur.timer += time;                          // 计时器累加
        } else {
            if (ui->ui_blur.blur_cur < ui->ui_blur.blur_tgt) {
                ui->ui_blur.blur_cur++;
                ui->ui_blur.timer = 0;
            }
            if (ui->ui_blur.blur_cur > ui->ui_blur.blur_tgt) {
                ui->ui_blur.blur_cur--;
                ui->ui_blur.timer = 0;
            }
        }
        ui->ui_blur.blur_end = (ui->ui_blur.blur_cur == ui->ui_blur.blur_tgt);
    }
    tm_fb_set_color(&ui->fb, 0);
    tm_fb_blur(&ui->fb, (tm_blur)(ui->ui_blur.blur_cur));
    tm_fb_set_color(&ui->fb, 1);
    return ui->ui_blur.blur_end;
}

#define TM_PAGE_CALL(page, method)                                                              \
    (((tm_page*)page)->methods->method(page)) // 使用这个宏调用方法，避免传错指针

static void tm_indicator_proc(tm_ui* ui, tm_page* p) { // 同样为了提高对消息的响应能力，tgt和cur的坐标设置需要在Animation函数调用前，
    // 这样可以保证在soft动态刷新时一有消息进入，就可以接力到anim_is_finish到false使状态机全速运行
    TM_PAGE_CALL(p, indicator_ctrl);
    // indicator anim
    tm_animation(&ui->indicator.x, tm_ui_anim_ms(ui, TM_ANI_INDICATOR), ui->time,
                     &(ui->anim_is_finish));
    tm_animation(&ui->indicator.y, tm_ui_anim_ms(ui, TM_ANI_INDICATOR), ui->time,
                     &(ui->anim_is_finish));
    tm_animation(&ui->indicator.w, tm_ui_anim_ms(ui, TM_ANI_INDICATOR), ui->time,
                     &(ui->anim_is_finish));
    tm_animation(&ui->indicator.h, tm_ui_anim_ms(ui, TM_ANI_INDICATOR), ui->time,
                     &(ui->anim_is_finish));
}

static void tm_scroll_bar_proc(tm_ui* ui, tm_page* p) { // 同样为了提高对消息的响应能力，tgt和cur的坐标设置需要在Animation函数调用前，
    // 这样可以保证在soft动态刷新时一有消息进入，就可以接力到anim_is_finish到false使状态机全速运行
    if (tm_page_is_popup(p) &&
        tm_func_do_nothing ==
            p->methods
                ->scrollbar_ctrl) { // 如果当前页面是弹窗页面同时弹窗页面没有对scrollbar操作的话
        TM_PAGE_CALL(p->last_page,
                        scrollbar_ctrl); // 做背景页面的bar函数(因为部分页面有bar但弹窗不带bar函数)
    }
    TM_PAGE_CALL(p, scrollbar_ctrl);
    tm_animation(&ui->scrollBar.y, tm_ui_anim_ms(ui, TM_ANI_LIST), ui->time,
                     &(ui->anim_is_finish));
}

// One frame of the IN state: the next page's entry animation and the fade-in (a popup
// instead animates in over the page underneath while that one dims).
static void tm_ui_step_in(tm_ui* ui) {
    bool ret_in, ret_blur; // 两个都要调用，不能放进一个短路表达式
    if (tm_page_is_popup(ui->in_page)) {
        tm_page_show(ui->current_page); // 绘制背景页面
        ret_blur = tm_blur_proc(ui, ui->time); // 渐隐
        ret_in = tm_page_in(ui->in_page); // 同时做弹窗的IN动画
    } else {
        ret_in = tm_page_in(ui->in_page); // 调用页面进入函数
        ret_blur = tm_blur_proc(ui, ui->time); // 同时进行渐显操作
    }
    if (ret_in && ret_blur) {
        ui->current_page = ui->in_page; // 在page_proc过程中，in cur page保持一致
        ui->state = TM_STATE_PROC;      // 两者都完成时进入页面显示和React
    }
}

// Whether a change between these two pages slides rather than fades.
static bool tm_slide_wanted(const tm_ui* ui, tm_page_addr from, tm_page_addr to) {
    return ui->params.style.animate && ui->params.style.transition == TM_TRANSITION_SLIDE &&
           from != NULL && to != NULL && from != to && !tm_page_is_popup(from) && !tm_page_is_popup(to);
}

// Starts sliding from the current page to ui->in_page; dir 1 goes deeper, -1 back.
static void tm_slide_start(tm_ui* ui, int8_t dir) {
    ui->slide = (tm_anim){.pos_cur = 0, .pos_tgt = ui->fb.w};
    ui->slide_dir = dir;
    ui->slide_settled = false;
    ui->slide_indicator = ui->indicator; // the old page leaves with its highlight
    ui->slide_bar = ui->scrollBar;
    TM_PAGE_CALL(ui->in_page, in_para_init);
    ui->state = TM_STATE_SLIDE;
}

// One frame of a slide: both pages, side by side, each with its highlight and bar.
// Leaves the framebuffer origin on the new page for the indicator and bar that follow.
static void tm_ui_step_slide(tm_ui* ui) {
    if (ui->slide.pos_cur == 0 && !ui->slide_settled) {
        // The new page arrives as it will look: finish its own entry animation and put
        // its highlight and bar in place, drawing off screen.
        const uint16_t time = ui->time;
        uint8_t done = true;
        ui->fb.ox = 2 * ui->fb.w;
        ui->time = 1000; // longer than any animation: each one completes in a step
        for (int i = 0; i < 8 && !ui->slide_settled; i++)
            ui->slide_settled = tm_page_in(ui->in_page);
        TM_PAGE_CALL(ui->in_page, scrollbar_ctrl);
        TM_PAGE_CALL(ui->in_page, indicator_ctrl);
        tm_animation(&ui->scrollBar.y, 0, ui->time, &done);
        tm_animation(&ui->indicator.x, 0, ui->time, &done);
        tm_animation(&ui->indicator.y, 0, ui->time, &done);
        tm_animation(&ui->indicator.w, 0, ui->time, &done);
        tm_animation(&ui->indicator.h, 0, ui->time, &done);
        ui->time = time;
    }
    tm_animation(&ui->slide, tm_ui_anim_ms(ui, TM_ANI_SLIDE), ui->time, &ui->anim_is_finish);
    const int16_t d = ui->slide.pos_cur, w = ui->fb.w;
    ui->fb.ox = ui->slide_dir > 0 ? -d : d;
    tm_page_show(ui->current_page);
    const tm_indicator indicator = ui->indicator;
    const tm_scrollbar bar = ui->scrollBar;
    ui->indicator = ui->slide_indicator; // where they were when the slide began
    ui->scrollBar = ui->slide_bar;
    TM_PAGE_CALL(ui->current_page, scrollbar_ctrl);
    TM_PAGE_CALL(ui->current_page, indicator_ctrl);
    ui->indicator = indicator;
    ui->scrollBar = bar;
    ui->fb.ox = ui->slide_dir > 0 ? w - d : d - w;
    // `in` runs a page's entry once; after it reports done the page draws with `show`.
    if (ui->slide_settled)
        tm_page_show(ui->in_page);
    else
        ui->slide_settled = tm_page_in(ui->in_page);
    if (ui->slide_settled && d == ui->slide.pos_tgt) {
        ui->current_page = ui->in_page;
        ui->state = TM_STATE_PROC;
    }
}

// 注意，在以下状态机中，in out状态中，cur_page都指向当前页面，in_page都指向要进入的页面，
// 在进入page_proc状态之前，完成in到cur_page的交接，在不同情况下切换p_all的值，表示当前与ui中全局元素交互的应该时哪个页面
// 所以，对页面方法有要求就是，每个页面的in,inparainit,show,react函数内只能对自己的类变量和变量进行操作，
// 如果要操作到全局ui对象里的变量，像indicator和ScrollerBar的话，需要使用类变量暂存，再在对应的indicator_ctrl和scrollbar_ctrl方法中进行操作
void tm_ui_proc(tm_ui* ui, uint16_t elapsed_ms) {
    ui->time = elapsed_ms;
    if (!tm_msg_que_is_empty(&(ui->msg_queue)))
        ui->is_motionless = false; // 有消息输入时开启动画
    if (ui->current_page != NULL && !(ui->is_motionless)) {
        bool ret_in = false,
             ret_blur = false; // 由于短路原则不直接在if中使用两个函数(存放in和 blur函数的返回值)
        tm_page* p_all =
            (tm_page*)ui
                ->current_page; // 把当前使用的与全局指示器和bar交互的页面是哪个(默认是当前页面)
        tm_fb_clear(&ui->fb);
        switch (ui->state) {
        case TM_STATE_IN: // 进入动画(全页面渐显，弹窗是同时做渐隐)
            p_all = (tm_page*)ui->in_page; // 与指示器交互的是要进入的页面
            tm_ui_step_in(ui);
            break;
        case TM_STATE_PROC: // 页面显示
            ui->anim_is_finish =
                true; // 假设每一次show过程的动画都是完成的，如果有动画未完成就会被置成false.
            ui->slide_is_finish =
                true; // 同样假设每一次show中的滚动动画都是完成的，如果有动画未完成就会被置成false.
            // 提高消息响应能力，先react再show
            p_all = (tm_page*)ui->current_page; // page显示与总页面交互的也是当前页面
            if (TM_PAGE_CALL(ui->current_page, react)) // 页面react(退出时返回true)
            {
                if (!tm_page_is_popup(ui->current_page))
                    ui->slide_dir = -1; // 返回上级
                if (tm_slide_wanted(ui, ui->current_page, ui->in_page))
                    tm_slide_start(ui, -1); // 返回上级：从左边滑入
                else if (tm_page_is_popup(ui->current_page))
                    tm_blur_para_init(ui, 0, tm_ui_anim_ms(ui, TM_ANI_FADE)); // 弹窗退出是做渐显参数初始化
                else
                    tm_blur_para_init(ui, 4, tm_ui_anim_ms(ui, TM_ANI_FADE)); // 渐隐参数初始化
                if (ui->state != TM_STATE_SLIDE)
                    ui->state = TM_STATE_OUT;
            }
            tm_page_show(ui->current_page); // 页面处理任务函数
            break;
        case TM_STATE_SLIDE:
            p_all = (tm_page*)ui->in_page; // 指示器和滚动条跟着新页面
            tm_ui_step_slide(ui);
            break;
        case TM_STATE_OUT: // 页面退出函数(全页面是做渐隐，弹窗页面做渐显)
            if (tm_page_is_popup(ui->current_page)) {
                p_all = (tm_page*)ui->in_page; // 与指示器交互的页面应该是背景页面
                tm_page_show(ui->in_page); // 绘制背景页面
                ret_blur = tm_blur_proc(ui, ui->time);
                ret_in = tm_page_in(ui->current_page); // 这个是弹窗out动画函数，起始参数不一样而已，绘制与in完全一样，所以直接用in
                if (ret_blur && ret_in) { // 渐显和out动画完成
                    ui->current_page = ui->in_page;
                    ui->state = TM_STATE_PROC; // 弹窗退出时不用做in动画
                }
            } else {                              // 普通全页面只需要完全渐隐即可
                p_all = (tm_page*)ui->in_page; // 渐隐时的指示器动画换成即将要进入页面的动画
                if (!ui->params.style.animate) { // 不做渐隐：这一帧就开始画下一个页面
                    tm_blur_para_init(ui, 0, 0);
                    ui->ui_blur.blur_cur = 0;
                    TM_PAGE_CALL(ui->in_page, in_para_init);
                    ui->state = TM_STATE_IN;
                    tm_ui_step_in(ui);
                    break;
                }
                tm_page_show(ui->current_page); // 由于clearbuff一直清空buff，所以需要重新绘制上一个页面用于渐隐
                if (tm_blur_proc(ui, ui->time)) { // 渐隐函数调用，返回true时，渐隐完成
                    tm_blur_para_init(ui, 0, tm_ui_anim_ms(ui, TM_ANI_FADE)); // 渐显参数初始化
                    if (ui->params.style.transition == TM_TRANSITION_CASCADE)
                        ui->ui_blur.blur_cur = 0; // 依次滑入：新页面直接清晰地进来
                    TM_PAGE_CALL(ui->in_page,
                                    in_para_init); // 调用要进入页面的IN动画参数初始化
                    ui->state = TM_STATE_IN;
                }
            }
            break;
        }
        tm_scroll_bar_proc(ui, p_all); // 侧边条处理
        tm_indicator_proc(ui, p_all); // 指示器处理
        ui->fb.ox = 0;
// 软件刷新和硬件刷新的判断都必须放在所有动画绘制之后
        if (ui->state == TM_STATE_IN || ui->state == TM_STATE_OUT || ui->state == TM_STATE_SLIDE)
            ui->is_motionless = false; // in/out动画阶段不可能静止(动画结束就跳转show动画了)
        else if (ui->state == TM_STATE_PROC) // show阶段判断两种动画是否完成
            ui->is_motionless =
                ui->anim_is_finish && ui->slide_is_finish; // anim和slide动画都完成了
        tm_ui_flush_changed(ui);
    }
}

/**
 * @brief 跳转到指定页面(用于关联确认上下级页面关系)
 *
 * @param self_page 当前页面对象的地址
 * @param terminate_page 目标页面地址
 */
void tm_jump_to_page(tm_page_addr self_page_addr, tm_page_addr terminate_page) {
    if (terminate_page == NULL)
        return;
    tm_ui* ui = ((tm_page*)terminate_page)->ui;
    // 关联上级页面并跳转页面
    if (terminate_page != NULL) {
        if (tm_page_is_inited(terminate_page)) { // 检查页面是否初始化过
            tm_page* p_ter = (tm_page*)terminate_page;
            // 跳到主页面或上级页面算返回(侧滑方向相反)，要在改写 last_page 之前判断
            int8_t dir = terminate_page == ui->home_page ? -1 : 1;
            for (tm_page* up = self_page_addr ? ((tm_page*)self_page_addr)->last_page : NULL, *prev = self_page_addr;
                 up != NULL && up != prev && dir > 0; prev = up, up = up->last_page)
                if (up == p_ter)
                    dir = -1;
            p_ter->last_page = self_page_addr;  // 给要跳转的页面绑定上级页面
            if (!tm_page_is_popup(terminate_page))
                ui->slide_dir = dir; // 侧滑和依次滑入的方向
            ui->is_motionless = false; // 唤醒空闲的UI，回调之外的跳转也要立刻开始动画
            ui->in_page = terminate_page; // 暂存要跳转的页面，等待渐隐结束赋值给cur_page跳转
            if (tm_page_is_popup(terminate_page)) { // 如果要跳转的页面是弹窗的话
                TM_PAGE_CALL(ui->in_page, in_para_init); // 弹窗in动画目标参数初始化
                tm_blur_para_init(ui, ui->win_bg_blur,
                    tm_ui_anim_ms(ui, TM_ANI_FADE)); // 渐隐参数初始化(是弹窗的话上一个页面只消失一半)
                ui->state = TM_STATE_IN; // 弹窗页面在in渐隐中同时做in动画
            } else if (tm_slide_wanted(ui, self_page_addr, terminate_page)) {
                tm_slide_start(ui, dir); // 侧滑：新旧页面同时移动
            } else {
                tm_blur_para_init(ui, 4, tm_ui_anim_ms(ui, TM_ANI_FADE)); // 渐隐参数初始化(不是弹窗的话上一个页面完全消失)
                ui->state = TM_STATE_OUT; // 改变页面，启动渐隐
            }
        } else {
            TM_LOG_E("The Page will jump is not inited!!");
        }
    }
}

/**
 * @brief 得到当前页面的地址
 *
 * @param terminate_page The address of the page to change to.
 */
