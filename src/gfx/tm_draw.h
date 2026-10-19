#ifndef TM_GFX_TM_DRAW_H_
#define TM_GFX_TM_DRAW_H_

#include "gfx/tm_fb.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A canvas is a clipping window on a framebuffer. Coordinates passed to the
 * drawing functions are relative to (start_x, start_y); nothing outside the
 * window or the framebuffer is touched.
 */
typedef struct {
    tm_fb* fb;
    int16_t start_x; // 画布起始点x坐标
    int16_t start_y; // 画布起始点y坐标
    int16_t w;       // 画布宽度
    int16_t h;       // 画布高度
} tm_canvas;

// Writes 8 vertical pixels starting at (x, y) with the framebuffer's pen.
void tm_canvas_write_byte(tm_canvas* canvas, int16_t x, int16_t y, uint8_t val);

void tm_canvas_draw_point(tm_canvas* canvas, int16_t x, int16_t y);
void tm_canvas_draw_line(tm_canvas* canvas, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
void tm_canvas_draw_line_v(tm_canvas* canvas, int16_t x, int16_t y_start, int16_t y_end);
void tm_canvas_draw_line_h(tm_canvas* canvas, int16_t x_start, int16_t x_end, int16_t y);
void tm_canvas_draw_dashed_line_v(tm_canvas* canvas, int16_t x, int16_t y_start, int16_t y_end,
                                  uint8_t DashedStyle, uint8_t Unit_Length);
void tm_canvas_draw_dashed_line_h(tm_canvas* canvas, int16_t x_start, int16_t x_end, int16_t y,
                                  uint8_t DashedStyle, uint8_t Unit_Length);
void tm_canvas_draw_rbox(tm_canvas* canvas, int16_t x_start, int16_t y_start, int16_t width,
                         int16_t height, uint8_t r);
void tm_canvas_draw_rbox_empty(tm_canvas* canvas, int16_t x_start, int16_t y_start, int16_t width,
                               int16_t height, uint8_t r);
// Only the corners of a box, each `r` long.
void tm_canvas_draw_box_right_angle(tm_canvas* canvas, int16_t x_start, int16_t y_start,
                                    int16_t width, int16_t height, uint8_t r);
// Page-major bitmap (same layout as the framebuffer); color 0 draws it inverted.
void tm_canvas_draw_bmp(tm_canvas* canvas, int16_t x, int16_t y, int16_t width, int16_t height,
                        const uint8_t* BMP, uint8_t color);

#ifdef __cplusplus
}
#endif

#endif
