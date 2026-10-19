#ifndef TM_POPUPS_TM_MSG_POPUP_H_
#define TM_POPUPS_TM_MSG_POPUP_H_

#include "core/tm_page.h"

#ifdef __cplusplus
extern "C" {
#endif

// 类型定义
struct tm_msg_popup_vars {
    tm_canvas canvas;            // 弹窗画布
    int16_t indicator_h_temp; // 暂时保存指示器的高度值在ind_ctrl中统一给指示器赋值
    uint16_t autonewline_need_h; // 弹窗文本是否需要滑动
};
typedef struct {
    tm_page page;                // 页面基础信息
    tm_option* bg_opt;           // 自动获取到的背景页面的选项的指针
    const char* content;            // 显示的消息内容
    int16_t str_start_y;      // 字符串书写的起始位置
    uint16_t str_move_step;   // 字符串移动时的步长
    bool auto_get_bg_opt : 1; // 弹窗页面是否自动获得lastPage中的select_opt
    struct tm_msg_popup_vars vars; // 本页面的状态
} tm_msg_popup;
/**
 * @brief 初始化消息窗口页面
 *
 * @param mw 指向消息窗口的指针
 * @param content 消息内容
 * @param auto_get_bg_opt 是否自动获取背景选项
 * @param move_step 移动步长
 * @param cb 回调函数
 *
 * @attention auto_get_bg_opt 若为true，会自动获取背景页面选中项的content覆盖弹窗内的content文本
 */
void tm_msg_popup_init(tm_msg_popup* mw, tm_ui* ui, const char* content, bool auto_get_bg_opt, uint16_t move_step,
                           tm_page_cb cb);
/**
 * @brief 设置消息窗口的显示内容
 *
 * @param mw 指向消息窗口的指针
 * @param content 要显示的消息内容
 * @return true 设置成功
 * @return false 设置失败
 *
 * @attention 确保消息窗口页面初始化过(建议只在回调函数中调用此函数)
 */
bool tm_msg_popup_set_content(tm_msg_popup* mw, const char* content);
/**
 * @brief 向上滑动消息窗口文本
 *
 * @param mw 指向消息窗口的指针
 * @return true 如果成功滑动
 * @return false 如果无法滑动
 *
 * @attention 确保消息窗口页面初始化过(建议只在回调函数中调用此函数)
 */
bool tm_msg_popup_slide_up_txt(tm_msg_popup* mw); // 返回值表示能否成功滑动
/**
 * @brief 向下滑动消息窗口文本
 *
 * @param mw 指向消息窗口的指针
 * @return true 如果成功滑动
 * @return false 如果无法滑动
 *
 * @attention 确保消息窗口页面初始化过(建议只在回调函数中调用此函数)
 */
bool tm_msg_popup_slide_down_txt(tm_msg_popup* mw);

#ifdef __cplusplus
}
#endif

#endif
