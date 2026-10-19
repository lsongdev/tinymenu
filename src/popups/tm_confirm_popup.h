#ifndef TM_POPUPS_TM_CONFIRM_POPUP_H_
#define TM_POPUPS_TM_CONFIRM_POPUP_H_

#include "core/tm_page.h"

#ifdef __cplusplus
extern "C" {
#endif

// 类型定义
struct tm_confirm_popup_vars {
    tm_anim win_y;               // 弹窗的竖直动画
    tm_canvas canvas;               // 弹窗画布
    tm_slide_str btn_ss;             // 按键滑动字符
    uint16_t autonewline_need_h; // 弹窗文本是否需要滑动
};
typedef struct {
    tm_page page;              // 页面基础信息
    tm_option* bg_opt;         // 自动获取到的背景页面的选项的指针
    const char* content;          // 显示的消息内容
    const char* str_left;         // 左侧按键内容
    const char* str_right;        // 右侧按键内容
    int16_t str_start_y;    // 字符串书写的起始位置
    uint16_t str_move_step; // 字符串移动时的步长
    bool conf_ret
        : 1; // 确认弹窗的结果，如果使能自动获取的话，这个成员和content的值都会自动被置成lastPage的选中项
    bool auto_get_bg_opt : 1; // 弹窗页面是否自动获得lastPage中的select_opt
    bool auto_set_bg_opt : 1; // 弹窗页面是否自动设置lastPage中的select_opt相关的值
    struct tm_confirm_popup_vars vars; // 本页面的状态
} tm_confirm_popup;
/**
 * @brief 初始化配置窗口页面
 *
 * @param cw 配置窗口对象指针
 * @param content 页面内容
 * @param str_left 左侧按钮文本(NULl指针输入默认为Yes)
 * @param str_right 右侧按钮文本(NULl指针输入默认为No)
 * @param conf_ret 配置返回值
 * @param auto_get_bg_opt 自动获取背景选项
 * @param auto_set_bg_opt 自动设置背景选项
 * @param move_step 移动步长
 * @param cb 回调函数
 * @attention auto_get_bg_opt
 * 为true,会自动获取背景页面选中项的val和content覆盖conf_ret和content。auto_set_bg_opt同理
 */
void tm_confirm_popup_init(tm_confirm_popup* cw, tm_ui* ui, const char* content, const char* str_left, const char* str_right,
                            bool conf_ret, bool auto_get_bg_opt, bool auto_set_bg_opt,
                            uint16_t move_step, tm_page_cb cb);
/**
 * @brief 向上滑动配置窗口页面文本
 *
 * @param cw 配置窗口对象指针
 * @return true 成功滑动
 * @return false 失败
 * @attention 确保消息窗口页面初始化过(建议只在回调函数中调用此函数)
 */
bool tm_confirm_popup_slide_up_txt(tm_confirm_popup* cw);

/**
 * @brief 向下滑动配置窗口页面文本
 *
 * @param cw 配置窗口对象指针
 * @return true 成功滑动
 * @return false 失败
 * @attention 确保消息窗口页面初始化过(建议只在回调函数中调用此函数)
 */
bool tm_confirm_popup_slide_down_txt(tm_confirm_popup* cw);

/**
 * @brief 切换配置窗口页面按钮
 *
 * @param cw 配置窗口对象指针
 * @attention 确保消息窗口页面初始化过(建议只在回调函数中调用此函数)
 */
void tm_confirm_popup_toggle_btn(tm_confirm_popup* cw);

#ifdef __cplusplus
}
#endif

#endif
