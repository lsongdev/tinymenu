#ifndef TM_POPUPS_TM_VALUE_POPUP_H_
#define TM_POPUPS_TM_VALUE_POPUP_H_

#include "core/tm_page.h"

#ifdef __cplusplus
extern "C" {
#endif

// 类型定义
struct tm_value_popup_vars {
    tm_anim win_y;    // 弹窗的竖直动画
    tm_slide_str text_ss; // 文本的滚动动画
    tm_slide_str val_ss;  // 数值的滚动动画
    tm_slide_str min_ss;  // 最小值的滚动动画
    tm_slide_str max_ss;  // 最大值的滚动动画
};
typedef struct {
    tm_page page;                // 页面基础信息
    tm_option* bg_opt;           // 自动获取到的背景页面的选项的指针
    const char* text;               // 要显示的文本
    int32_t val;              // 弹窗数值
    int32_t min;              // 弹窗最小值
    int32_t max;              // 弹窗最大值
    int32_t step;             // 弹窗步长
    bool auto_get_bg_opt : 1; // 弹窗页面是否自动获得lastPage中的select_opt
    bool auto_set_bg_opt : 1; // 弹窗页面是否自动设置lastPage中的select_opt相关的值
    struct tm_value_popup_vars vars; // 本页面的状态
} tm_value_popup;
/**
 * @brief Valwin页面初始化
 * @param vw Valwin实例指针
 * @param text 显示的文本
 * @param init_val 初始值
 * @param min 最小值
 * @param max 最大值
 * @param step 步进值
 * @param auto_get_bg_opt 自动获取背景选项
 * @param auto_set_bg_opt 自动设置背景选项
 * @param cb 回调函数
 * @attention auto_get_bg_opt 为true会自动获取背景页面选中项的text作为文本，获取val作为init_val,
 * auto_set_bg_opt同理
 */
void tm_value_popup_init(tm_value_popup* vw, tm_ui* ui, const char* text, int32_t init_val, int32_t min, int32_t max,
                           int32_t step, bool auto_get_bg_opt, bool auto_set_bg_opt,
                           tm_page_cb cb);
/**
 * @brief 设置Valwin页面的最小值、步进值和最大值
 * @param vw Valwin实例指针
 * @param min 最小值
 * @param step 步进值
 * @param max 最大值
 * @attention 确保min<= step <= max，
 */
void tm_value_popup_set_min_step_max(tm_value_popup* vw, int32_t min, int32_t step, int32_t max);
// Moves the value one step up (dir > 0) or down (dir < 0); false at the end of the range.
bool tm_value_popup_step(tm_value_popup* vw, int8_t dir);

#ifdef __cplusplus
}
#endif

#endif
