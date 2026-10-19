#ifndef TM_CORE_TM_PROGRESS_H_
#define TM_CORE_TM_PROGRESS_H_

#include "core/tm_page.h"

#ifdef __cplusplus
extern "C" {
#endif

/* A progress bar: a frame with an inverted fill. Determinate bars fill in
 * proportion to val between min and max and slide to each new value; busy
 * bars, for work of unknown length, run a segment through the frame. Pages
 * draw it inside their own layout; tm_progress_popup puts one in a popup. */
typedef struct {
    int32_t min, max, val;
    bool busy;
    uint16_t busy_ms; /* phase of the busy segment */
    tm_anim fill;     /* filled width in pixels */
} tm_progress;

void tm_progress_init(tm_progress* bar, int32_t min, int32_t max, int32_t val);
/* Clamps val to min..max and leaves busy mode. */
void tm_progress_set(tm_progress* bar, int32_t val);
void tm_progress_set_busy(tm_progress* bar, bool busy);
/* 0..100; 0 when min == max. */
uint8_t tm_progress_percent(const tm_progress* bar);

/* Frame of a w x h bar at (x, y); the fill area is 2px inside it. */
void tm_progress_draw_frame(tm_ui* ui, int16_t x, int16_t y, int16_t w, int16_t h);
/* Fill width for val inside a bar w px wide. */
int16_t tm_progress_fill_width(int32_t min, int32_t max, int32_t val, int16_t w);
/* The inverted fill, at a rectangle the caller chooses (it may be animating). */
void tm_progress_draw_fill(tm_ui* ui, int16_t x, int16_t y, int16_t w, int16_t h);
/* Frame and fill, advancing the fill or busy animation by ui->time. Keeps the
 * UI redrawing while anything moves. */
void tm_progress_draw(tm_progress* bar, tm_ui* ui, int16_t x, int16_t y, int16_t w, int16_t h);

#ifdef __cplusplus
}
#endif
#endif
