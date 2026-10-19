#ifndef TM_GFX_TM_FB_H_
#define TM_GFX_TM_FB_H_

#include "tm_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A 1-bit framebuffer in page-major order, the native layout of SSD1306-class
 * controllers: pixel (x, y) is bit (y % 8) of buf[(y / 8) * w + x]. The caller
 * owns the memory, tm_fb_size(w, h) bytes.
 */

#define TM_PEN_NORMAL 0x00
#define TM_PEN_XOR (!TM_PEN_NORMAL)
#define TM_PEN_BLACK 0x00            // 写1的点显示为黑色(灭)
#define TM_PEN_WHITE (!TM_PEN_BLACK) // 写1的点显示为白色(亮)

typedef struct {
    bool color_mode : 1;     // 颜色模式，正常(|//&)还是异或 normal=0;xor=1;
    bool color : 1;          // 画笔颜色(指前景色，背景色默认与前景色反色)(1白色，0黑色)
    bool rev_color_flag : 1; // 是否反色绘制标志，0不反色，1反色。
} tm_pen;

typedef struct {
    uint8_t* buf;
    int16_t w;
    int16_t h;
    tm_pen pen;
    int16_t ox; // added to every x written: draws a page shifted, clipped at the edges
} tm_fb;

typedef enum {
    TM_BLUR_0 = 0x00, // 完全显示
    TM_BLUR_1,
    TM_BLUR_2,
    TM_BLUR_3,
    TM_BLUR_4, // 完全虚化
} tm_blur;

static inline size_t tm_fb_pages_of(int16_t h) { return (size_t)((h + 7) / 8); }
static inline size_t tm_fb_size(int16_t w, int16_t h) { return (size_t)w * tm_fb_pages_of(h); }
static inline int16_t tm_fb_pages(const tm_fb* fb) { return (int16_t)tm_fb_pages_of(fb->h); }

void tm_fb_init(tm_fb* fb, uint8_t* buf, int16_t w, int16_t h);

// Clears to the background colour of the current pen.
void tm_fb_clear(tm_fb* fb);

// 0: background, 1: foreground, 2: XOR.
void tm_fb_set_color(tm_fb* fb, uint8_t color);

// Swaps foreground and background (light theme).
void tm_fb_set_inverted(tm_fb* fb, bool inverted);

// Covers the whole buffer with a dither pattern of the given density.
void tm_fb_blur(tm_fb* fb, tm_blur blur);

// Writes 8 vertical pixels at column x of page `page` with the current pen; out of range is ignored.
void tm_fb_write_byte(tm_fb* fb, int16_t x, int16_t page, uint8_t val);

// Expands rows [y, y + rows) to row-major 16-bit pixels for colour displays: `on` where a
// pixel is set, `off` elsewhere. Each framebuffer pixel becomes a scale x scale block, so
// a 320x240 panel can show a UI laid out for 160x120 at scale 2. `out` holds
// (fb->w * scale) * (rows * scale) values. The colours are written as given: pass them
// already byte-swapped if the display wants big-endian RGB565.
void tm_fb_expand16(const tm_fb* fb, int16_t y, int16_t rows, uint8_t scale, uint16_t on, uint16_t off,
                    uint16_t* out);

#ifdef __cplusplus
}
#endif

#endif
