#ifndef TM_POPUPS_TM_DIGIT_POPUP_H_
#define TM_POPUPS_TM_DIGIT_POPUP_H_

#include "core/tm_page.h"

#ifdef __cplusplus
extern "C" {
#endif

// 类型定义
struct tm_digit_popup_vars {
    tm_anim win_y;      // 弹窗的竖直动画
    tm_slide_str text_ss;   // 文本的滚动动画
    tm_slide_str min_ss;    // 最小值的滚动动画
    tm_slide_str max_ss;    // 最大值的滚动动画
    int16_t num_w_temp; // 数字总宽度暂存
};

typedef struct {
    tm_page page;                // 页面基础信息
    tm_option* bg_opt;           // 自动获取到的背景页面的选项的指针
    const char* text;               // 要显示的文本
    int32_t val;              // 弹窗数值
    int32_t min;              // 弹窗最小值
    int32_t max;              // 弹窗最大值
    uint8_t selected_digit : 4;      // 选中位0-7
    tm_decimals dec_num : 3;   // 定点数的类型
    uint8_t editing : 1;     // digit弹窗是否有选中
    bool auto_get_bg_opt : 1; // 弹窗页面是否自动获得lastPage中的select_opt
    bool auto_set_bg_opt : 1; // 弹窗页面是否自动设置lastPage中的select_opt相关的值
    struct tm_digit_popup_vars vars; // 本页面的状态
} tm_digit_popup;
/**
 * @brief 设置数值选择窗口的最小值、最大值和小数位数
 * @param spw 指向DigitWin结构体的指针
 * @param min 最小值
 * @param max 最大值
 * @param decnum 小数位数
 * @attention 如果该页面使能自动获取背景页面opt的话，小数位数建议与opt的位数保持一致
 */
void tm_digit_popup_set_min_max_decimals(tm_digit_popup* spw, int32_t min, int32_t max,
                                           tm_decimals decnum);
/**
 * @brief 初始化数值选择窗口
 * @param spw 指向DigitWin结构体的指针
 * @param text 显示文本
 * @param init_val 初始值
 * @param dec_num 小数位数
 * @param min 最小值
 * @param max 最大值
 * @param auto_get_bg_opt 自动获取背景选项
 * @param auto_set_bg_opt 自动设置背景选项
 * @param cb 回调函数
 * @attention auto_get_bg_opt 为true会自动获取背景页面选中项的text作为文本，获取val作为init_val,
 * 获取opt的小数位数作为dec_num, auto_set_bg_opt同理
 */
void tm_digit_popup_init(tm_digit_popup* spw, tm_ui* ui, const char* text, int32_t init_val, tm_decimals dec_num,
                            int32_t min, int32_t max, bool auto_get_bg_opt, bool auto_set_bg_opt,
                            tm_page_cb cb);
/**
 * @brief 移动数值选择窗口的选择位
 * @param spw 指向DigitWin结构体的指针
 * @param right 移动方向(0表示向左,1表示向右)
 * @attention (建议只在回调函数中使用此函数)
 */
void tm_digit_popup_move_digit(tm_digit_popup* spw, bool right);
/**
 * @brief 切换数值选择窗口的选择状态
 * @param spw 指向DigitWin结构体的指针
 * @attention (建议只在回调函数中使用此函数)
 */
void tm_digit_popup_toggle_editing(tm_digit_popup* spw);
/**
 * @brief 改变数值选择窗口当前选择位的值
 * @param spw 指向DigitWin结构体的指针
 * @param direction 增加或减少(1表示增加,-1表示减少)
 * @return true 改变成功
 * @return false 改变失败
 * @attention (建议只在回调函数中使用此函数)
 */
bool tm_digit_popup_change_digit(tm_digit_popup* spw, int32_t direction);

#ifdef __cplusplus
}
#endif

#endif
