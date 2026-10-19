#include "gfx/tm_draw.h"

/**
 * @brief : void tm_canvas_write_byte(tm_canvas * canvas ,int16_t x, int16_t y, uint8_t val)
 * @param : canvas 画图窗口，x相对于画图原点的横坐标，y相对于画图原点的纵坐标，写入的一个字节
 * @attention : OLED_OK/OUT/ERR
 */
void tm_canvas_write_byte(tm_canvas* canvas, int16_t x, int16_t y, uint8_t val) {
    uint8_t n = 0, m = 0, temp1 = 0, temp2 = 0;
    int16_t real_y = 0, over_bit1 = 0, over_bit2 = 0;
    if (x > canvas->w || y > canvas->h || x < 0 || y < -7)
        return; // 超出窗口大小
    if (y + 7 > (canvas->h - 1))
        val &= (~(0xFF << (canvas->h - y))); // 超过窗口的部分不显示
    if (y >= -7 && y < 0)
        over_bit1 = (-1 * y);     // 没到窗口的部分也不显示
    real_y = canvas->start_y + y; // 增加了对canvas_y为负的时候的处理
    if (real_y < -7)
        return; // 整个字节都在缓冲区上方，没有可见像素(否则下面的取模和移位会越界)
    if (real_y >= -7 && real_y < 0) {
        over_bit2 = (-1 * real_y);
        val >>= MAX(over_bit1, over_bit2); // 处理超过窗口和buff部分的值
        n = 0;
        m = 0;
    } else {
        val &= (0xFF << over_bit1); // 去掉低的溢出位
        n = real_y / 8;
        m = real_y % 8;
    }
    temp1 = val << m;
    temp2 = (val >> (8 - m));
    // 在全局buff中第几个字节的第几位 ,取出低字节的位,取出高字节的位
    if (m == 0)
        tm_fb_write_byte(canvas->fb, canvas->start_x + x, n, val); // 恰好是整字节
    else if (m != 0) {
        tm_fb_write_byte(canvas->fb, canvas->start_x + x, n, temp1);
        tm_fb_write_byte(canvas->fb, canvas->start_x + x, n + 1, temp2);
    }
}

/**
 * @brief : void tm_canvas_draw_line_v(tm_canvas *canvas,int16_t x, int16_t y_start, int16_t y_end)
 * @param : 往指定窗口中画线，注意，y_end 必须大于y_start
 * @attention : None
 */
void tm_canvas_draw_line_v(tm_canvas* canvas, int16_t x, int16_t y_start, int16_t y_end) {
    if (y_start < 0)
        y_start = 0;
    if (y_end < 0)
        y_end = 0;
    if (y_start > canvas->w)
        y_start = canvas->w;
    if (y_end > canvas->w)
        y_end = canvas->w;
    if (x > canvas->w || x < 0)
        return;
    if (y_start > y_end) {
        int16_t index = y_end;
        y_end = y_start;
        y_start = index;
    }
    uint8_t n = 0, m = 0;
    if ((y_end - y_start) < 7)
        tm_canvas_write_byte(canvas, x, y_start, ~(0xFF << (y_end - y_start + 1)));
    else {
        uint8_t i = 0;
        n = (y_end - y_start) / 8;
        m = (y_end - y_start) % 8;
        for (i = 0; i < n; i++)
            tm_canvas_write_byte(canvas, x, y_start + i * 8, 0xFF);
        tm_canvas_write_byte(canvas, x, y_start + i * 8, ~(0xFF << (m + 1)));
    }
}

/**
 * @brief : void tm_canvas_draw_dashed_line_v(tm_canvas *canvas,int16_t x, int16_t y_start, int16_t
 * y_end)
 * @param : 往指定窗口中画虚线，注意，y_end 必须大于y_start
 * @attention : None
 */
void tm_canvas_draw_dashed_line_v(tm_canvas* canvas, int16_t x, int16_t y_start, int16_t y_end,
                                   uint8_t DashedStyle, uint8_t Unit_Length) {
    if (y_start < 0)
        y_start = 0;
    if (y_end < 0)
        y_end = 0;
    if (y_start > canvas->w)
        y_start = canvas->w;
    if (y_end > canvas->w)
        y_end = canvas->w;
    if (x > canvas->w || x < 0)
        return;
    if (y_start > y_end) {
        int16_t index = y_end;
        y_end = y_start;
        y_start = index;
    }
    for (uint8_t i = 0; i < 3; i++) {
        if (Unit_Length == (0x01 << i)) {
            Unit_Length <<= 1;
            DashedStyle |= (DashedStyle << (0x01 << i));
        }
    }
    if (Unit_Length == 8) {
        uint8_t n = 0, m = 0;
        uint8_t i = 0;
        n = (y_end - y_start) / 8;
        m = (y_end - y_start) % 8;
        for (i = 0; i < n; i++)
            tm_canvas_write_byte(canvas, x, y_start + i * 8, DashedStyle);
        tm_canvas_write_byte(canvas, x, y_start + i * 8, DashedStyle & (0xFF >> (7 - m)));
    } else {
        for (int16_t i = 0; i <= (y_end - y_start); i++)
            tm_canvas_write_byte(canvas, x, y_start + i,
                                   (DashedStyle >> (i % Unit_Length)) & 0x01);
    }
}

/**
 * @brief : void tm_canvas_draw_line_h(tm_canvas * canvas, int16_t x_start, int16_t x_end, int16_t y)
 * @param : 注意x_end > x_start
 */
void tm_canvas_draw_line_h(tm_canvas* canvas, int16_t x_start, int16_t x_end, int16_t y) {
    if (x_start < 0)
        x_start = 0;
    if (x_end < 0)
        x_end = 0;
    if (x_start > canvas->w)
        x_start = canvas->w;
    if (x_end > canvas->w)
        x_end = canvas->w;
    if (y > canvas->h || y < 0)
        return;
    if (x_start > x_end) {
        int16_t index = x_end;
        x_end = x_start;
        x_start = index;
    }
    for (int16_t i = 0; i <= (x_end - x_start); i++)
        tm_canvas_write_byte(canvas, x_start + i, y, 0x01);
}

/**
 * @brief : void tm_canvas_draw_dashed_line_h(tm_canvas * canvas, int16_t x_start, int16_t x_end,
 * int16_t y)
 * @param : 注意x_end > x_start
 */
void tm_canvas_draw_dashed_line_h(tm_canvas* canvas, int16_t x_start, int16_t x_end, int16_t y,
                                  uint8_t DashedStyle, uint8_t Unit_Length) {
    if (x_start < 0)
        x_start = 0;
    if (x_end < 0)
        x_end = 0;
    if (x_start > canvas->w)
        x_start = canvas->w;
    if (x_end > canvas->w)
        x_end = canvas->w;
    if (y > canvas->h || y < 0)
        return;
    if (x_start > x_end) {
        int16_t index = x_end;
        x_end = x_start;
        x_start = index;
    }
    for (int16_t i = 0; i <= (x_end - x_start); i++)
        tm_canvas_write_byte(canvas, x_start + i, y, (DashedStyle >> (i % Unit_Length)) & 0x01);
}

/**
 * @brief : static void canvas_draw_rbox(tm_canvas *canvas, int16_t x_start, int16_t y_start,
 * int16_t width, int16_t height, uint8_t r, bool fill)
 * @param : r 倒角像素的大小
 * @attention : 绘制倒角矩形,注意倒角不能大于宽或高的1/2(鲁棒性真的好，还带了倒角检查)
 */
static void canvas_draw_rbox(tm_canvas* canvas, int16_t x_start, int16_t y_start, int16_t width,
                                 int16_t height, uint8_t r, bool fill) {
    if (width < 1 || height < 1)
        return;
    uint8_t max_r = width > height ? (height - 1) >> 1 : (width - 1) >> 1;
    if (r > max_r)
        r = max_r;
    uint8_t cir_r = r;
    for (int16_t i = 0; i < height; i++) {
        if (fill || i == 0 || i == height - 1) {
            tm_canvas_draw_line_h(canvas, x_start + r, x_start + width - r - 1, y_start + i);
        } else {
            tm_canvas_write_byte(canvas, x_start + r, y_start + i, 0x01);
            tm_canvas_write_byte(canvas, x_start + width - r - 1, y_start + i, 0x01);
        }
        if (i < cir_r && r > 0)
            r--;
        if (i >= (height - cir_r - 1) && (r + 1) <= max_r)
            r++;
    }
}

/**
 * @brief : void tm_canvas_draw_rbox(tm_canvas *canvas, int16_t x_start, int16_t y_start, int16_t
 * width, int16_t height, uint8_t r)
 * @param : r 倒角像素的大小
 * @attention : 绘制倒角矩形,注意倒角不能大于宽或高的1/2
 */
void tm_canvas_draw_rbox(tm_canvas* canvas, int16_t x_start, int16_t y_start, int16_t width,
                           int16_t height, uint8_t r) {
    canvas_draw_rbox(canvas, x_start, y_start, width, height, r, true);
}

/**
 * @brief : void tm_canvas_draw_rbox_empty(tm_canvas *canvas, int16_t x_start, int16_t y_start,
 * int16_t width, int16_t height, uint8_t r)
 * @param :  在窗口内绘制空心的倒角矩形,r 倒角像素的大小
 * @attention : 绘制倒角矩形,注意倒角不能大于宽或高的1/2
 */
void tm_canvas_draw_rbox_empty(tm_canvas* canvas, int16_t x_start, int16_t y_start, int16_t width,
                                int16_t height, uint8_t r) {
    canvas_draw_rbox(canvas, x_start, y_start, width, height, r, false);
}

/**
 * @brief 画矩形的四个直角
 *
 * @param canvas 绘制的窗口
 * @param x_start 矩形左上角x坐标
 * @param y_start 矩形左上角y坐标
 * @param width 矩形宽度
 * @param height 矩形高度
 * @param r 直角长度
 */
void tm_canvas_draw_box_right_angle(tm_canvas* canvas, int16_t x_start, int16_t y_start, int16_t width,
                                    int16_t height, uint8_t r) {
    if (width < 1 || height < 1 || r < 1)
        return;
    uint8_t max_r = width > height ? (height - 1) >> 1 : (width - 1) >> 1;
    if (r > max_r)
        r = max_r;
    tm_canvas_draw_line_h(canvas, x_start, x_start + r - 1, y_start);
    tm_canvas_draw_line_h(canvas, x_start + width - r, x_start + width - 1, y_start);
    tm_canvas_draw_line_h(canvas, x_start, x_start + r - 1, y_start + height - 1);
    tm_canvas_draw_line_h(canvas, x_start + width - r, x_start + width - 1, y_start + height - 1);

    tm_canvas_draw_line_v(canvas, x_start, y_start, y_start + r - 1);
    tm_canvas_draw_line_v(canvas, x_start, y_start + height - r, y_start + height - 1);
    tm_canvas_draw_line_v(canvas, x_start + width - 1, y_start, y_start + r - 1);
    tm_canvas_draw_line_v(canvas, x_start + width - 1, y_start + height - r,
                            y_start + height - 1);
}

/**
 * @brief : void tm_canvas_draw_bmp(tm_canvas * canvas, int16_t x, int16_t y, int16_t width, int16_t
 * height,const uint8_t * BMP, uint8_t color)
 * @param : heigh must be a total times of 8, the color is convenient for draw inverse-color BMP
 */
void tm_canvas_draw_bmp(tm_canvas* canvas, int16_t x, int16_t y, int16_t width, int16_t height,
                          const uint8_t* BMP, uint8_t color) {
    if (BMP == NULL)
        return; // 如果是NULL，直接返回
    uint16_t n = height / 8, m = height % 8;
    for (uint16_t i = 0; i < n; i++) {
        for (uint16_t j = 0; j < width; j++) {
            if (x + j > canvas->w)
                break;
            if (y + i * 8 > canvas->h)
                return;
            if (color == 1)
                tm_canvas_write_byte(canvas, x + j, y + i * 8, (BMP[i * width + j]));
            else
                tm_canvas_write_byte(canvas, x + j, y + i * 8, ~(BMP[i * width + j]));
        }
    }
    if (m != 0) {
        for (int16_t j = 0; j < width; j++) {
            if (x + j > canvas->w)
                return;
            if (color == 1)
                tm_canvas_write_byte(canvas, x + j, y + n * 8,
                                       (BMP[n * width + j] & (~(0xFF << m))));
            else
                tm_canvas_write_byte(canvas, x + j, y + n * 8,
                                       ~(BMP[n * width + j] & (~(0xFF << m))));
        }
    }
}

/**
 * @brief : void tm_canvas_draw_point(tm_canvas * canvas, int16_t x, int16_t y)
 * @param : //画点函数尚未测试
 */
void tm_canvas_draw_point(tm_canvas* canvas, int16_t x, int16_t y) {
    tm_canvas_write_byte(canvas, x, y, 0x01);
}

/**
 * @brief : void tm_canvas_draw_line(tm_canvas* canvas,int16_t x1, int16_t y1, int16_t x2, int16_t
 * y2)
 * @param : (x1,y1)为起点，(x2,y2)为终点
 * @attention : 使用Bresenham算法进行画直线，任意方向(包括竖线)
 * @author : Sheep
 * @date : 23/10/31
 */
void tm_canvas_draw_line(tm_canvas* canvas, int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    int16_t x_small = 0, x_big = 0, temp = 0, y_small = 0, y_big = 0;
    int32_t dx = 0, dy = 0, p = 0; // deltax 和deltay
    if (ABS(y2 - y1) < ABS(x2 - x1)) // 斜率绝对值小于 1：沿 x 走(竖线走下面沿 y 的分支)
    {
        if (x2 > x1) {
            x_small = x1;
            x_big = x2;
        } else {
            x_small = x2;
            x_big = x1;
            temp = y1;
            y1 = y2;
            y2 = temp;
        } // 默认与 xsmall对应的y是y1
        dx = x_big - x_small;
        dy = ((y1 - y2) > 0) ? (y1 - y2) : (y2 - y1);
        p = (dy << 1) - dx;
        while (x_small <= x_big) {
            tm_canvas_write_byte(canvas, x_small, y1, 0x01);
            if (p > 0) {
                if (y1 < y2)
                    y1++;
                else
                    y1--;
                p = p + (dy << 1) - (dx << 1);
            } else {
                p = p + (dy << 1);
            }
            x_small++;
        }
    } else // p>=1
    {
        if (y2 > y1) {
            y_small = y1;
            y_big = y2;
        } else {
            y_small = y2;
            y_big = y1;
            temp = x1;
            x1 = x2;
            x2 = temp;
        } // 默认与y_small对应的是x1;
        dx = (x1 > x2) ? (x1 - x2) : (x2 - x1);
        dy = y_big - y_small;
        p = (dx << 1) - dy;
        while (y_small <= y_big) {
            tm_canvas_write_byte(canvas, x1, y_small, 0x01);
            if (p > 0) {
                if (x1 < x2)
                    x1++;
                else
                    x1--;
                p = p + (dx << 1) - (dy << 1);
            } else {
                p = p + (dx << 1);
            }
            y_small++;
        }
    }
}
