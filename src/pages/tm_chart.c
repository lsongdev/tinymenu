#include "core/tm_ui.h"
#include "pages/tm_chart.h"

#define FONT DEFAULT_LIST_TEXT_FONT
#define FONT_H GET_FNOT_H(FONT)
#define HEAD_H (FONT_H + 3)       // 竖柱和折线的标题行：选中项的标签和数值，下面一条点线
#define LABEL_H (FONT_H + 1)      // 柱子下的标签
#define ROW_H (FONT_H + 4)        // 横条的一行
#define BAR_W DEFAULT_LIST_BAR_W  // 横条的滚动条
#define MIN_SLOT 3                // 一根柱子(连同间隔)最少占的宽度
#define GROW_FULL 256

static bool horizontal_items(const tm_chart_page* c) { return c->type != TM_CHART_HBAR; }

static void value_text(const tm_option* o, char* buf, size_t len) {
    if (o->decimals)
        tm_format_fixed(buf, len, o->val, o->decimals);
    else
        tm_format_int(buf, len, o->val);
}

// The value axis: fixed, or the values' range stretched to include 0.
static void value_range(const tm_chart_page* c, int32_t* lo, int32_t* hi) {
    if (c->min != c->max) {
        *lo = MIN(c->min, c->max);
        *hi = MAX(c->min, c->max);
        return;
    }
    int32_t a = 0, b = 0;
    for (uint8_t i = 0; i < c->item_num; i++) {
        a = MIN(a, c->option_array[i].val);
        b = MAX(b, c->option_array[i].val);
    }
    *lo = a;
    *hi = a == b ? a + 1 : b;
}

static int32_t clamp(int32_t v, int32_t lo, int32_t hi) { return v < lo ? lo : v > hi ? hi : v; }

// The value drawn while bars grow in from the base line.
static int32_t grown(const tm_chart_page* c, int32_t v, int32_t base) {
    return base + (int32_t)((int64_t)(v - base) * c->vars.grow.pos_cur / GROW_FULL);
}

//-------- columns and lines
static int16_t slot_w(const tm_chart_page* c) {
    return MAX(c->page.ui->fb.w / MAX(c->item_num, 1), MIN_SLOT);
}
static int16_t content_w(const tm_chart_page* c) { return slot_w(c) * c->item_num; }
static int16_t first_x(const tm_chart_page* c) { // 放得下时居中
    return MAX((c->page.ui->fb.w - content_w(c)) / 2, 0);
}
static bool labels_fit(const tm_chart_page* c) {
    for (uint8_t i = 0; i < c->item_num; i++)
        if ((int16_t)tm_text_width(c->option_array[i].text, FONT) + 2 > slot_w(c)) // 留出间隔
            return false;
    return true;
}
static int16_t plot_top(void) { return HEAD_H + 1; }
static int16_t plot_bottom(const tm_chart_page* c) {
    return c->page.ui->fb.h - 1 - (labels_fit(c) ? LABEL_H : 0);
}
static int16_t value_y(const tm_chart_page* c, int32_t v, int32_t lo, int32_t hi) {
    const int16_t top = plot_top(), bottom = plot_bottom(c);
    return bottom - (int16_t)((int64_t)(clamp(v, lo, hi) - lo) * (bottom - top) / (hi - lo));
}

static void draw_columns(tm_chart_page* c) {
    tm_ui* ui = c->page.ui;
    char buf[16];
    int32_t lo, hi;
    value_range(c, &lo, &hi);
    const int32_t base = clamp(0, lo, hi);
    const int16_t slot = slot_w(c), gap = slot >= 8 ? 2 : 1, x0 = first_x(c) - c->vars.scroll.pos_cur;
    const int16_t base_y = value_y(c, base, lo, hi), bottom = plot_bottom(c);
    const bool labels = labels_fit(c);
    // Header: the selected item's label and value.
    const tm_option* sel = &c->option_array[c->select_item];
    value_text(sel, buf, sizeof(buf));
    const int16_t value_w = (int16_t)tm_text_width(buf, FONT);
    tm_canvas head = {.fb = &ui->fb, .start_x = 0, .start_y = 1, .w = ui->fb.w - value_w - 4, .h = FONT_H};
    tm_canvas_draw_str(&head, 1, 0, FONT, sel->text);
    tm_canvas_draw_str(&ui->w_all, ui->fb.w - value_w - 1, 1, FONT, buf);
    tm_canvas_draw_dashed_line_h(&ui->w_all, 0, ui->fb.w - 1, HEAD_H - 1, 0x01, 2);
    tm_canvas_draw_line_h(&ui->w_all, 0, ui->fb.w - 1, base_y); // 基线
    int16_t last_x = 0, last_y = 0;
    for (uint8_t i = 0; i < c->item_num; i++) {
        const int16_t x = x0 + i * slot;
        const int16_t y = value_y(c, grown(c, c->option_array[i].val, base), lo, hi);
        if (c->type == TM_CHART_LINE) {
            const int16_t cx = x + slot / 2;
            if (i > 0)
                tm_canvas_draw_line(&ui->w_all, last_x, last_y, cx, y);
            if (slot >= 6)
                tm_canvas_draw_rbox(&ui->w_all, cx - 1, y - 1, 3, 3, 0);
            last_x = cx;
            last_y = y;
        } else if (x + slot > 0 && x < ui->fb.w) {
            const int16_t y0 = MIN(y, base_y), y1 = MAX(y, base_y);
            tm_canvas_draw_rbox(&ui->w_all, x + gap / 2, y0, slot - gap, y1 - y0 + 1, 0);
        }
        if (labels && x + slot > 0 && x < ui->fb.w) {
            const char* t = c->option_array[i].text;
            tm_canvas_draw_str(&ui->w_all, x + (slot - (int16_t)tm_text_width(t, FONT)) / 2 + 1,
                               bottom + 2, FONT, t);
        }
    }
}

//-------- rows (TM_CHART_HBAR)
static int16_t rows_overflow(const tm_chart_page* c) {
    return c->item_num * ROW_H > c->page.ui->fb.h;
}
static int16_t rows_right(const tm_chart_page* c) {
    return c->page.ui->fb.w - (rows_overflow(c) ? BAR_W + 2 : 1);
}

static void draw_rows(tm_chart_page* c) {
    tm_ui* ui = c->page.ui;
    char buf[16];
    int32_t lo, hi;
    value_range(c, &lo, &hi);
    const int32_t base = clamp(0, lo, hi);
    int16_t label_w = 0, value_w = 0;
    for (uint8_t i = 0; i < c->item_num; i++) {
        value_text(&c->option_array[i], buf, sizeof(buf));
        label_w = MAX(label_w, (int16_t)tm_text_width(c->option_array[i].text, FONT));
        value_w = MAX(value_w, (int16_t)tm_text_width(buf, FONT));
    }
    label_w = MIN(label_w, ui->fb.w / 3);
    const int16_t right = rows_right(c), bar_x0 = 2 + label_w + 3, bar_x1 = right - value_w - 4;
    const int16_t span = MAX(bar_x1 - bar_x0, 1);
    const int16_t base_x = bar_x0 + (int16_t)((int64_t)(base - lo) * span / (hi - lo));
    for (uint8_t i = 0; i < c->item_num; i++) {
        const int16_t y = i * ROW_H - c->vars.scroll.pos_cur;
        if (y + ROW_H <= 0 || y >= ui->fb.h)
            continue;
        const tm_option* o = &c->option_array[i];
        tm_canvas label = {.fb = &ui->fb, .start_x = 2, .start_y = y + 2, .w = label_w, .h = FONT_H};
        tm_canvas_draw_str(&label, 0, 0, FONT, o->text);
        const int16_t vx = bar_x0 + (int16_t)((int64_t)(clamp(grown(c, o->val, base), lo, hi) - lo) * span / (hi - lo));
        tm_canvas_draw_rbox(&ui->w_all, MIN(vx, base_x), y + 3, MAX(ABS(vx - base_x), 1), ROW_H - 6, 0);
        value_text(o, buf, sizeof(buf));
        tm_canvas_draw_str(&ui->w_all, right - 1 - (int16_t)tm_text_width(buf, FONT), y + 2, FONT, buf);
    }
    if (rows_overflow(c)) { // 滚动条
        const int16_t total = c->item_num * ROW_H, view = ui->fb.h, x = ui->fb.w - 1 - BAR_W / 2;
        tm_canvas_draw_dashed_line_v(&ui->w_all, x, 0, view - 1, 0x01, 2);
        const int16_t thumb = MAX(view * view / total, 4);
        const int16_t ty = (int16_t)((int32_t)c->vars.scroll.pos_cur * (view - thumb) / (total - view));
        tm_canvas_draw_rbox(&ui->w_all, ui->fb.w - BAR_W, ty, BAR_W, thumb, 0);
    }
}

//-------- page
// Scrolls just enough to show the selected item whole.
static void follow_selection(tm_chart_page* c) {
    const bool along = horizontal_items(c);
    const int16_t size = along ? slot_w(c) : ROW_H;
    const int16_t view = along ? c->page.ui->fb.w : c->page.ui->fb.h;
    const int16_t total = along ? content_w(c) : c->item_num * ROW_H;
    const int16_t pos = c->select_item * size;
    int16_t tgt = c->vars.scroll.pos_tgt;
    if (pos < tgt)
        tgt = pos;
    if (pos + size > tgt + view)
        tgt = pos + size - view;
    c->vars.scroll.pos_tgt = clamp(tgt, 0, MAX(total - view, 0));
}

static void draw(tm_chart_page* c) {
    tm_ui* ui = c->page.ui;
    tm_animation(&c->vars.scroll, tm_ui_anim_ms(ui, TM_ANI_LIST), ui->time, &ui->anim_is_finish);
    tm_animation(&c->vars.grow, tm_ui_anim_ms(ui, TM_ANI_LIST) * 2, ui->time, &ui->anim_is_finish);
    if (horizontal_items(c))
        draw_columns(c);
    else
        draw_rows(c);
}

static void in_init(tm_page_addr addr) {
    tm_chart_page* c = addr;
    follow_selection(c);
    c->vars.scroll.pos_cur = c->vars.scroll.pos_tgt;
    c->vars.grow = (tm_anim){.pos_cur = 0, .pos_tgt = GROW_FULL}; // 柱子从基线长出来
}

static bool chart_in(tm_page_addr addr) {
    tm_chart_page* c = addr;
    draw(c);
    return c->vars.grow.pos_cur == c->vars.grow.pos_tgt;
}

static void show(tm_page_addr addr) { draw(addr); }

bool tm_chart_page_move(tm_chart_page* c, int16_t delta) {
    const int16_t n = c->item_num;
    int16_t to = c->select_item + delta;
    if (to < 0 || to >= n) {
        if (!c->page.ui->params.loop_param[TM_LOOP_LIST] || n == 0)
            return false;
        to = (to + n) % n;
    }
    c->select_item = (uint8_t)to;
    follow_selection(c);
    return true;
}

void tm_chart_page_select(tm_chart_page* c, uint8_t index) {
    if (index >= c->item_num)
        return;
    c->select_item = index;
    follow_selection(c);
}

static bool react(tm_page_addr addr) {
    tm_chart_page* c = addr;
    tm_ui* ui = c->page.ui;
    bool ret = false;
    tm_msg msg = tm_msg_que_read(&ui->msg_queue);
    if (c->page.auto_input) {
        // Columns run sideways (up goes forward, as on the tile page); rows run down, as in a list.
        const int8_t step = horizontal_items(c) ? tm_msg_amount(msg) : tm_msg_step(msg);
        if (step)
            tm_chart_page_move(c, step);
        else if (msg == TM_MSG_RETURN) {
            tm_page_return(addr);
            ret = true;
        }
        if (msg != TM_MSG_NONE && c->page.cb)
            c->page.cb(&c->page, msg);
    } else if (msg != TM_MSG_NONE && c->page.cb) {
        ret = c->page.cb(&c->page, msg);
        if (ret)
            tm_page_return(addr);
    }
    return ret;
}

static void indicator(tm_page_addr addr) {
    tm_chart_page* c = addr;
    tm_ui* ui = c->page.ui;
    if (c->type == TM_CHART_BAR) {
        // 选中的那根柱子外扩 1 像素：只框数据本身，反色时柱子变成空心，高度不变(框整列的话，
        // 反色会把柱子上方的空白变成一大块白色，看起来像数据)
        int32_t lo, hi;
        value_range(c, &lo, &hi);
        const int16_t slot = slot_w(c), gap = slot >= 8 ? 2 : 1;
        const int16_t y = value_y(c, c->option_array[c->select_item].val, lo, hi);
        const int16_t base_y = value_y(c, clamp(0, lo, hi), lo, hi);
        ui->indicator.x.pos_tgt =
            first_x(c) + c->select_item * slot - c->vars.scroll.pos_tgt + gap / 2 - 1;
        ui->indicator.y.pos_tgt = MIN(y, base_y) - 1;
        ui->indicator.w.pos_tgt = slot - gap + 2;
        ui->indicator.h.pos_tgt = ABS(y - base_y) + 3;
        // 上下边直接贴着柱子：数值实时变化(如频谱)时框不会落后；左右换柱子仍有动画
        ui->indicator.y.pos_cur = ui->indicator.y.pos_tgt;
        ui->indicator.h.pos_cur = ui->indicator.h.pos_tgt;
    } else if (horizontal_items(c)) { // 折线：选中的那一列
        ui->indicator.x.pos_tgt = first_x(c) + c->select_item * slot_w(c) - c->vars.scroll.pos_tgt;
        ui->indicator.y.pos_tgt = plot_top();
        ui->indicator.w.pos_tgt = slot_w(c);
        ui->indicator.h.pos_tgt = plot_bottom(c) - plot_top() + 1;
    } else { // 选中的那一行
        ui->indicator.x.pos_tgt = 0;
        ui->indicator.y.pos_tgt = c->select_item * ROW_H - c->vars.scroll.pos_tgt;
        ui->indicator.w.pos_tgt = rows_right(c) + 1;
        ui->indicator.h.pos_tgt = ROW_H;
    }
    tm_ui_draw_highlight(ui, 1);
}

static const tm_page_methods methods = {
    .in = chart_in, .in_para_init = in_init, .show = show, .react = react,
    .indicator_ctrl = indicator, .scrollbar_ctrl = tm_func_do_nothing,
};

void tm_chart_page_init(tm_chart_page* c, tm_ui* ui, uint8_t item_num, tm_option* option_array,
                        tm_chart_type type, tm_page_cb cb) {
    c->page.page_type = TM_PAGE_CHART;
    tm_page_init(c, ui, cb);
    c->page.methods = &methods;
    c->item_num = item_num;
    c->option_array = option_array;
    c->select_item = 0;
    c->icon_w = c->icon_h = 0;
    c->type = type;
    c->min = c->max = 0;
    memset(&c->vars, 0, sizeof(c->vars));
    c->vars.grow.pos_cur = c->vars.grow.pos_tgt = GROW_FULL;
    for (uint8_t i = 0; i < item_num; i++) {
        option_array[i].order = i;
        if (!option_array[i].text)
            option_array[i].text = TM_POPUP_TEXT_DEFAULT;
    }
}

void tm_chart_page_refresh(tm_chart_page* c) {
    follow_selection(c);
    c->page.ui->is_motionless = false;
}
