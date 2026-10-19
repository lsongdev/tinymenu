#include "gfx/tm_text.h"

#include <string.h>

/*
函数：void tm_canvas_draw_char(tm_canvas *canvas,int16_t x, int16_t y ,uint8_t size, char c)
参数：[in] win， x，y ,size(12,16,24) c
返回值：目前递增到的x的位置
说明：绘制单个字符
*/
int16_t tm_canvas_draw_char(tm_canvas* canvas, int16_t x, int16_t y, tm_font font, char c) {
    c = c - ' '; // 得到偏移值
    switch (font.WidthHeight) {
    case 68: // 8号字6*8
        for (uint8_t i = 0; i < 6; i++) {
            tm_canvas_write_byte(canvas, x, y, font.table[c * 6 + i]);
            x++;
            if (x > canvas->w)
                break; // 已经超出边框没必要再写了
        }
        break;

    case 612: // 12号字6*12
        for (uint8_t i = 0; i < 6; i++) {
            tm_canvas_write_byte(canvas, x, y, font.table[c * 2 * 6 + i]);
            tm_canvas_write_byte(canvas, x, y + 8, font.table[(c * 2 + 1) * 6 + i]);
            x++;
            if (x > canvas->w)
                break; // 已经超出边框没必要再写了
        }
        break;

    case 712: // 12号字7*12
        for (uint8_t i = 0; i < 7; i++) {
            tm_canvas_write_byte(canvas, x, y, font.table[c * 2 * 7 + i]);
            tm_canvas_write_byte(canvas, x, y + 8, font.table[(c * 2 + 1) * 7 + i]);
            x++;
            if (x > canvas->w)
                break; // 已经超出边框没必要再写了
        }
        break;

    case 816: // 16号字8x16
        for (uint8_t i = 0; i < 8; i++) {
            tm_canvas_write_byte(canvas, x, y, font.table[c * 2 * 8 + i]);
            tm_canvas_write_byte(canvas, x, y + 8, font.table[(c * 2 + 1) * 8 + i]);
            x++;
            if (x > canvas->w)
                break; // 已经超出边框没必要再写了
        }
        break;

    case 1224: // 24号字12*24
        for (uint8_t i = 0; i < 12; i++) {
            tm_canvas_write_byte(canvas, x, y, font.table[c * 3 * 12 + i]);
            tm_canvas_write_byte(canvas, x, y + 8, font.table[(c * 3 + 1) * 12 + i]);
            tm_canvas_write_byte(canvas, x, y + 16, font.table[(c * 3 + 2) * 12 + i]);
            x++;
            if (x > canvas->w)
                break; // 已经超出边框没必要再写了
        }
        break;
    default:
        break;
    }
    return x;
}

/**
 * @brief : void tm_canvas_draw_str(tm_canvas *canvas, int16_t x, int16_t y, tm_font font, uint8_t
 * *str)
 * @param : win指定窗口，x，y相对于窗口的坐标，str 字符串
 */
void tm_canvas_draw_str(tm_canvas* canvas, int16_t x, int16_t y, tm_font font, const char* str) {
    int16_t cur_x = x, cur_y = y;
    while (*str != '\0') {
        tm_canvas_draw_char(canvas, cur_x, cur_y, font, (uint8_t)*str);
        cur_x += font.Width;
        str++;
        if (cur_x > canvas->w || cur_y > canvas->h)
            break; // 已经到边了没必要再写了
    }
}

// Gap between the end of the text and its next copy in TM_SLIDE_LOOP, in characters.
#define TM_SLIDE_LOOP_GAP 3

void tm_slide_str_draw(tm_slide_str* ss, int16_t y, tm_font font, uint16_t elapsed_ms) {
    const int16_t text_w = (int16_t)tm_text_width(ss->str, font);
    if (ss->canvas.w >= text_w) {
        ss->slide_enable = false;   // 放得下，不需要滚动
        ss->slide_is_finish = true;
        ss->str_start_x = 0;
    }
    if (ss->slide_enable && ss->slide_mode != TM_SLIDE_OFF) {
        ss->slide_is_finish = false; // 滚动中一定是未完成的
        // One tick per TM_SLIDE_TICK_MS of elapsed time, so the speed does not depend on the frame rate.
        ss->elapsed_ms += elapsed_ms;
        while (ss->slide_enable && ss->elapsed_ms >= TM_SLIDE_TICK_MS) {
            ss->elapsed_ms -= TM_SLIDE_TICK_MS;
            if (ss->start_delay) { // 停顿(开始前，或到末尾后)
                ss->start_delay--;
                continue;
            }
            if (ss->at_tail) { // TM_SLIDE_ONCE: 停顿结束，回到开头并结束
                ss->at_tail = false;
                ss->str_start_x = 0;
                ss->slide_enable = false;
                ss->slide_is_finish = true;
                break;
            }
            ss->str_start_x -= ss->step;
            if (ss->slide_mode == TM_SLIDE_LOOP) {
                const int16_t period = (int16_t)(text_w + TM_SLIDE_LOOP_GAP * font.Width);
                if (ss->str_start_x <= -period) { // 下一份已经到了开头，回到一圈的起点
                    ss->str_start_x = 0;
                    ss->start_delay = TM_SLIDE_START_DELAY;
                }
            } else if (ss->str_start_x + text_w <= ss->canvas.w) { // 末尾露出来了
                ss->str_start_x = (int16_t)(ss->canvas.w - text_w);
                ss->at_tail = true;
                ss->start_delay = TM_SLIDE_START_DELAY;
            }
        }
    }
    tm_canvas_draw_str(&(ss->canvas), ss->str_start_x, y, font, ss->str);
    if (ss->slide_mode == TM_SLIDE_LOOP && ss->str_start_x < 0)
        tm_canvas_draw_str(&(ss->canvas), ss->str_start_x + text_w + TM_SLIDE_LOOP_GAP * font.Width,
                           y, font, ss->str);
}

/**
 * @brief : Resets the sliding string parameters, 可用于页面切换时使ss重新等待enable滚动
 * @param : ss - Pointer to the tm_slide_str structure to reset.
 * @return : None
 */
void tm_slide_str_reset(tm_slide_str* ss) {
    ss->str_start_x = 0;
    ss->slide_is_finish = false;
    ss->slide_enable = false;
    ss->at_tail = false;
    ss->start_delay = TM_SLIDE_START_DELAY;
    ss->elapsed_ms = 0;
}

/**
 * @brief : tm_canvas_draw_lines(tm_canvas *canvas, int16_t x, int16_t y, tm_font font,
 * uint8_t *str, uint8_t lineSpacing)
 * @param : win指定窗口，x，y相对于窗口的坐标，str 字符串,lineSpacing 行间距
 */
void tm_canvas_draw_lines(tm_canvas* canvas, int16_t x, int16_t y, tm_font font, const char* str,
                                     uint8_t lineSpacing) {
    int16_t cur_x = x, cur_y = y;
    while (*str != '\0') {
        if (*str == '\n') {
            str++;
            cur_x = x;
            cur_y += (font.Height + lineSpacing);
            continue;
        }
        tm_canvas_draw_char(canvas, cur_x, cur_y, font, (uint8_t)*str);
        cur_x += font.Width;
        str++;
        if (cur_x > canvas->w || cur_y > canvas->h)
            continue; // 已经到边了没必要再写了
    }
}

// Finds the line that starts at `str` in text wrapped to `cols` characters: sets
// *len to the characters to draw and returns where the next line starts (NULL after
// the last). Lines break at '\n', else at the last space that fits, else mid-word.
static const char* tm_text_next_line(const char* str, int16_t cols, int16_t* len) {
    if (cols < 1)
        cols = 1;
    int16_t n = 0, last_space = -1;
    while (n < cols && str[n] != '\0' && str[n] != '\n') {
        if (str[n] == ' ')
            last_space = n;
        n++;
    }
    if (str[n] == '\0') {
        *len = n;
        return NULL;
    }
    if (str[n] == '\n') {
        *len = n;
        return str + n + 1;
    }
    if (str[n] == ' ') // the word ends exactly at the edge
        last_space = n;
    const char* next;
    if (last_space > 0) {
        *len = last_space;
        next = str + last_space;
    } else {
        *len = n;
        next = str + n;
    }
    while (*next == ' ')
        next++; // a wrapped line does not start with spaces
    return *next ? next : NULL;
}

void tm_canvas_draw_wrapped(tm_canvas* canvas, int16_t x, int16_t y, tm_font font,
                                     const char* str) {
    const int16_t cols = (int16_t)((canvas->w - x) / font.Width);
    while (str != NULL && y <= canvas->h) {
        int16_t len;
        const char* next = tm_text_next_line(str, cols, &len);
        for (int16_t i = 0; i < len; i++)
            tm_canvas_draw_char(canvas, (int16_t)(x + i * font.Width), y, font, (uint8_t)str[i]);
        y += (int16_t)(font.Height + TM_LINE_SPACING);
        str = next;
    }
}

/**
 * @brief : uint16_t tm_text_width(const char * str, tm_font font)
 * @param : 得到字符串的宽度
 * @attention : len
 */
uint16_t tm_text_width(const char* str, tm_font font) {
    return strlen(str) * font.Width;
}

/**
 * @brief : uint16_t tm_text_height(const char * str, tm_font font)
 * @param : 得到字符串的高度
 * @attention : len
 */
uint16_t tm_text_height(const char* str, tm_font font) {
    uint8_t lines = 1;
    if (str == NULL)
        return 0;
    while (*str != '\0') {
        if (*str == '\n')
            lines++;
        str++;
    }
    return lines * font.Height + (lines - 1) * TM_LINE_SPACING;
}

uint16_t tm_text_height_wrapped(int16_t canvas_w, const char* str, tm_font font) {
    if (str == NULL)
        return 0;
    uint16_t lines = 0;
    while (str != NULL) {
        int16_t len;
        str = tm_text_next_line(str, (int16_t)(canvas_w / font.Width), &len);
        lines++;
    }
    return lines * font.Height + (lines - 1) * TM_LINE_SPACING;
}
