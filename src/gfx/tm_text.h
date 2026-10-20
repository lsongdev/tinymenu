#ifndef TM_GFX_TM_TEXT_H_
#define TM_GFX_TM_TEXT_H_

#include "gfx/tm_draw.h"
#include "gfx/tm_font.h"

#ifdef __cplusplus
extern "C" {
#endif

// How text that does not fit scrolls once it is enabled.
typedef enum {
    TM_SLIDE_ONCE = 0x00, // 滚到末尾，停一下，回到开头后停止(之后界面可以静止)
    TM_SLIDE_LOOP,        // 首尾相接持续滚动，每圈在开头停一下
    TM_SLIDE_OFF,         // 不滚动，只显示开头
} tm_slide_mode;

// Text that scrolls horizontally inside its canvas when it does not fit.
typedef struct {
    uint8_t slide_enable : 1;     // 是否使能字符串滚动
    uint8_t slide_is_finish : 1;  // 单次滚动是否结束标志位
    uint8_t at_tail : 1;          // TM_SLIDE_ONCE 已滚到末尾，停顿后回到开头
    tm_slide_mode slide_mode : 2; // 滚动模式
    uint8_t start_delay;          // 开始滚动前等待的 tick 数
    uint16_t elapsed_ms;          // 未满一个 tick 的时间
    int16_t str_start_x;          // 用于滚动时记录字符串的起始位置
    tm_canvas canvas;             // 滚动画布
    const char* str;              // 要滚动的字符串
} tm_slide_str;

// Draws one character with its top-left corner at (x, y); returns the x after it.
int16_t tm_canvas_draw_char(tm_canvas* canvas, int16_t x, int16_t y, tm_font font, char c);
void tm_canvas_draw_str(tm_canvas* canvas, int16_t x, int16_t y, tm_font font, const char* str);
// Breaks lines at '\n' only.
void tm_canvas_draw_lines(tm_canvas* canvas, int16_t x, int16_t y, tm_font font, const char* str,
                          uint8_t lineSpacing);
// Breaks lines at '\n' and between words at the canvas edge (mid-word only when a word is wider
// than the canvas). tm_text_height_wrapped measures the same lines.
void tm_canvas_draw_wrapped(tm_canvas* canvas, int16_t x, int16_t y, tm_font font, const char* str);

// Scrolling advances one tick per TM_SLIDE_TICK_MS of elapsed time.
#define TM_SLIDE_TICK_MS 10
// Pixels moved per tick: every scrolling text in the UI goes at the same speed.
#define TM_SLIDE_STEP 1

// Draws the text, then advances its scroll position by `elapsed_ms`.
void tm_slide_str_draw(tm_slide_str* ss, int16_t y, tm_font font, uint16_t elapsed_ms);
void tm_slide_str_reset(tm_slide_str* ss);

uint16_t tm_text_width(const char* str, tm_font font);
uint16_t tm_text_height(const char* str, tm_font font);
uint16_t tm_text_height_wrapped(int16_t canvas_w, const char* str, tm_font font);

#ifdef __cplusplus
}
#endif

#endif
