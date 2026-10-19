#include "core/tm_progress.h"
#include "core/tm_ui.h"

#define BUSY_PERIOD_MS 1200 /* one pass of the busy segment */

void tm_progress_init(tm_progress* bar, int32_t min, int32_t max, int32_t val) {
    memset(bar, 0, sizeof(*bar));
    bar->min = MIN(min, max);
    bar->max = MAX(min, max);
    tm_progress_set(bar, val);
}

void tm_progress_set(tm_progress* bar, int32_t val) {
    bar->val = val < bar->min ? bar->min : val > bar->max ? bar->max : val;
    bar->busy = false;
}

void tm_progress_set_busy(tm_progress* bar, bool busy) {
    if (busy && !bar->busy)
        bar->busy_ms = 0;
    bar->busy = busy;
}

uint8_t tm_progress_percent(const tm_progress* bar) {
    if (bar->max == bar->min)
        return 0;
    return (uint8_t)((int64_t)(bar->val - bar->min) * 100 / (bar->max - bar->min));
}

int16_t tm_progress_fill_width(int32_t min, int32_t max, int32_t val, int16_t w) {
    int16_t inner = w - 4;
    if (max <= min || inner <= 0)
        return 0;
    val = val < min ? min : val > max ? max : val;
    return (int16_t)((int64_t)(val - min) * inner / (max - min));
}

void tm_progress_draw_frame(tm_ui* ui, int16_t x, int16_t y, int16_t w, int16_t h) {
    tm_canvas_draw_rbox_empty(&ui->w_all, x, y, w, h, tm_ui_radius(ui, DEFAULT_VAL_WIN_BAR_R));
}

void tm_progress_draw_fill(tm_ui* ui, int16_t x, int16_t y, int16_t w, int16_t h) {
    tm_fb_set_color(&ui->fb, 2); // 反色绘制
    tm_canvas_draw_rbox(&ui->w_all, x, y, w, h, tm_ui_radius(ui, DEFAULT_VAL_WIN_BAR_R));
    tm_fb_set_color(&ui->fb, 1);
}

void tm_progress_draw(tm_progress* bar, tm_ui* ui, int16_t x, int16_t y, int16_t w, int16_t h) {
    tm_progress_draw_frame(ui, x, y, w, h);
    int16_t inner_w = w - 4, inner_h = h - 4;
    if (inner_w <= 0 || inner_h <= 0)
        return;
    if (bar->busy) {
        /* A third of the bar enters on the left and leaves on the right. */
        bar->busy_ms = (uint16_t)((bar->busy_ms + ui->time) % BUSY_PERIOD_MS);
        int16_t seg = MAX(inner_w / 3, 2);
        int16_t pos = (int16_t)((int32_t)bar->busy_ms * (inner_w + seg) / BUSY_PERIOD_MS) - seg;
        tm_canvas c = {.fb = &ui->fb, .start_x = x + 2, .start_y = y + 2,
                       .w = inner_w, .h = inner_h};
        tm_fb_set_color(&ui->fb, 2);
        tm_canvas_draw_rbox(&c, pos, 0, seg, inner_h, tm_ui_radius(ui, DEFAULT_VAL_WIN_BAR_R));
        tm_fb_set_color(&ui->fb, 1);
        ui->anim_is_finish = false;
        return;
    }
    bar->fill.pos_tgt = tm_progress_fill_width(bar->min, bar->max, bar->val, w);
    tm_animation(&bar->fill, tm_ui_anim_ms(ui, TM_ANI_INDICATOR), ui->time, &ui->anim_is_finish);
    tm_progress_draw_fill(ui, x + 2, y + 2, bar->fill.pos_cur, inner_h);
}
