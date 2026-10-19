#include "gfx/tm_fb.h"

#include <string.h>

void tm_fb_init(tm_fb* fb, uint8_t* buf, int16_t w, int16_t h) {
    fb->buf = buf;
    fb->w = w;
    fb->h = h;
    fb->pen.color_mode = TM_PEN_NORMAL;
    fb->pen.color = TM_PEN_BLACK;
    fb->pen.rev_color_flag = 0;
    fb->ox = 0;
}

void tm_fb_set_color(tm_fb* fb, uint8_t color) {
    switch (color) {
    case 0:
        fb->pen.color_mode = TM_PEN_NORMAL;
        fb->pen.color = fb->pen.rev_color_flag ? TM_PEN_WHITE : TM_PEN_BLACK;
        break;
    case 1:
        fb->pen.color_mode = TM_PEN_NORMAL;
        fb->pen.color = fb->pen.rev_color_flag ? TM_PEN_BLACK : TM_PEN_WHITE;
        break;
    case 2:
        fb->pen.color_mode = TM_PEN_XOR;
        break;
    default:
        break;
    }
}

void tm_fb_set_inverted(tm_fb* fb, bool reverse) {
    fb->pen.rev_color_flag = reverse;
}

void tm_fb_clear(tm_fb* fb) {
    if (fb->pen.color == TM_PEN_WHITE) // 前景色为白色，背景刷黑色
        memset(fb->buf, 0x00, tm_fb_size(fb->w, fb->h));
    else
        memset(fb->buf, 0xFF, tm_fb_size(fb->w, fb->h));
}

void tm_fb_write_byte(tm_fb* fb, int16_t x, int16_t y, uint8_t val) {
    x += fb->ox;
    if (x > (fb->w - 1) || y > (tm_fb_pages(fb) - 1) || x < 0 || y < 0)
        return;
    if (fb->pen.color_mode == TM_PEN_NORMAL) {
        if (fb->pen.color == TM_PEN_WHITE)
            fb->buf[y * fb->w + x] |= val;
        else
            fb->buf[y * fb->w + x] &= (~val);
    } else
        fb->buf[y * fb->w + x] ^= val;
}

void tm_fb_blur(tm_fb* fb, tm_blur blur) {
    if (blur == TM_BLUR_0)
        return;
    static const uint8_t BLUR_PATTERN[5][2] = {
        // 定义模糊度查找表
        {0x00, 0x00}, // TM_BLUR_0
        {0x55, 0x00}, // TM_BLUR_1
        {0x55, 0xAA}, // TM_BLUR_2
        {0xFF, 0xAA}, // TM_BLUR_3
        {0xFF, 0xFF}  // TM_BLUR_4
    };
    uint8_t pattern_even = BLUR_PATTERN[blur][0];
    uint8_t pattern_odd = BLUR_PATTERN[blur][1];
    for (uint8_t page = 0; page < tm_fb_pages(fb); page++) { // 每8行一组进行处理
        for (uint16_t col = 0; col < fb->w; col += 2) {      // 一次处理两列
            tm_fb_write_byte(fb, col, page, pattern_even);
            tm_fb_write_byte(fb, col + 1, page, pattern_odd);
        }
    }
}

void tm_fb_expand16(const tm_fb* fb, int16_t y, int16_t rows, uint8_t scale, uint16_t on, uint16_t off,
                    uint16_t* out) {
    const size_t out_w = (size_t)fb->w * scale;
    for (int16_t row = y; row < y + rows; row++) {
        const uint8_t* page = fb->buf + (size_t)(row / 8) * fb->w;
        const uint8_t bit = (uint8_t)(1u << (row & 7));
        uint16_t* line = out;
        if (scale == 1) {
            for (int16_t x = 0; x < fb->w; x++) *out++ = (page[x] & bit) ? on : off;
            continue;
        }
        for (int16_t x = 0; x < fb->w; x++) {
            const uint16_t c = (page[x] & bit) ? on : off;
            for (uint8_t i = 0; i < scale; i++) *out++ = c;
        }
        for (uint8_t i = 1; i < scale; i++, out += out_w) memcpy(out, line, out_w * sizeof(*out));
    }
}
