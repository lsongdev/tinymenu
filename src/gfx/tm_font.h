#ifndef TM_GFX_TM_FONT_H_
#define TM_GFX_TM_FONT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*最大字体微软雅黑24 (32x41) */
#define MAX_HEIGHT_FONT 41
#define MAX_WIDTH_FONT 32
#define OFFSET_BITMAP


// !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
// ASCII
typedef struct tm_font {
    const uint8_t* table;
    uint16_t WidthHeight;
    uint16_t Width;
    uint16_t Height;
} tm_font;

// GB2312
typedef struct // 汉字字模数据结构
{
    unsigned char index[2];                                  // 汉字内码索引
    const char matrix[MAX_HEIGHT_FONT * MAX_WIDTH_FONT / 8]; // 点阵码数据
} tm_glyph_cn;

typedef struct {
    const tm_glyph_cn* table;
    uint16_t size;
    uint16_t ASCII_Width;
    uint16_t Width;
    uint16_t Height;
} tm_font_cn;

// extern tm_font Font_5_8;
extern const tm_font Font_6_8;
extern const tm_font Font_6_12;
extern const tm_font Font_7_12;
extern const tm_font Font_8_16;
// extern tm_font Font_14_20;
extern const tm_font Font_12_24;

extern tm_font_cn Font12CN;
extern tm_font_cn Font24CN;

#define GET_FNOT_H(fnot) (fnot.Height)
#define GET_FNOT_W(fnot) (fnot.Width)

#ifdef __cplusplus
}
#endif

#endif
