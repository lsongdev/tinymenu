#ifndef TM_PAGES_TM_WAVE_H_
#define TM_PAGES_TM_WAVE_H_

#include "core/tm_page.h"

#ifdef __cplusplus
extern "C" {
#endif

// 类型定义
// 波形页面的类变量
struct tm_wave_page_vars {
    tm_slide_str title_ss;    // 滑动标题
    tm_slide_str val_ss;      // 滑动数值
    tm_slide_str max_val_ss;  // 最大值滑动窗口
    tm_slide_str min_val_ss;  // 最小值滑动窗口
    tm_slide_str mid_val_ss;  // 中值滑动窗口
    tm_anim text_y;       // 标题的y坐标
    tm_anim y_axis_val_x; // 数值的y坐标
    uint16_t last_y;      // 上一次绘制的y坐标
    uint16_t cur_y;       // 本次绘制的y坐标
};
// 波形类型
typedef enum {
    TM_WAVE_SOLID = 0x00, // 实线波形
    TM_WAVE_DASH,         // 虚线波形
    TM_WAVE_FILL,         // 填充波形
    TM_WAVE_STEP,         // 阶梯：每次变化都是直角台阶，适合 GPIO 电平这类数字信号
} tm_wave_type;                // 波形类型
// 波形数据类
typedef struct tm_wave_data {
    const char* text;                 // 波形标题
    tm_wave_type type;          // 波形类型
    int16_t* data;              // 环形缓冲区，由调用者提供，长度至少为屏幕宽度
    uint16_t depth;             // data 的长度
    uint16_t idx_head;          // 队列头部指针/下标(最旧数据的位置)
    uint16_t idx_tail;          // 队列尾部指针/下标(最新数据的位置)
    uint16_t idx_show_max;      // 波形队列的最大指针/下标
    uint16_t idx_show_min;      // 波形队列的最小指针/下标
    uint16_t idx_show_head;     // 显示的波形数据的头指针(下标)
    uint16_t idx_show_tail;     // 显示的波形数据的尾指针(下标)
    int16_t range_max;           // 波形最大值
    int16_t range_min;           // 波形最小值
    tm_decimals decimals : 4;   // 小数位数
    bool stopped : 1;         // 波形停止滚动标志
    int16_t fixed_min, fixed_max; // 固定的纵轴范围(例如 GPIO 0~1、湿度 0~100)；相等(默认都为 0)时按数据自动缩放
} tm_wave_data;
// 波形页面类
typedef struct tm_wave_page {      // 列表页面
    tm_page page;                 // 基础页面信息
    tm_wave_data* waves; // 波形数据数组
    uint8_t wave_count;     // 波形数据个数(上面数组的大小)
    uint8_t selected;  // 选中的波形数据
    struct tm_wave_page_vars vars; // 本页面的状态
} tm_wave_page;
// ------波形页面接口函数
/**
 * @brief 初始化波形页面
 * @param wp 波形页面控制器指针
 * @param wave_num 波形数量
 * @param waves 波形数据数组
 * @param call_back 回调函数
 */
void tm_wave_page_init(tm_wave_page* wp, tm_ui* ui, uint8_t wave_num, tm_wave_data* waves,
                         tm_page_cb call_back);
/**
 * @brief 更新指定波形的数据值
 * @param wp 波形页面控制器指针
 * @param wave_num 波形编号
 * @param new_data 新的波形数据
 */
void tm_wave_page_push(tm_wave_page* wp, uint8_t wave_num, int16_t new_data);
// Shows another of the page's waves: delta waves on (wrapping), or wave index.
bool tm_wave_page_move(tm_wave_page* wp, int16_t delta);
void tm_wave_page_select(tm_wave_page* wp, uint8_t index);
// A stopped wave scrolls through the samples it kept: dir < 0 towards older ones, > 0
// towards newer. can_scroll says whether there is anything that way; scroll moves a sample.
bool tm_wave_page_can_scroll(tm_wave_page* wp, uint8_t wave_num, int8_t dir);
bool tm_wave_page_scroll(tm_wave_page* wp, uint8_t wave_num, int8_t dir);
/**
 * @brief 停止或重启波形显示
 * @param wp 波形页面控制器指针
 * @param wave_num 波形编号
 * @param stop 停止标志(true:停止, false:重启)
 * @attention 此函数只建议在回调函数中使用
 */
void tm_wave_page_stop(tm_wave_page* wp, uint8_t wave_num, bool stop);

#ifdef __cplusplus
}
#endif

#endif
