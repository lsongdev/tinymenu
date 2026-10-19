#ifndef TM_POPUPS_TM_LIST_POPUP_H_
#define TM_POPUPS_TM_LIST_POPUP_H_

#include "core/tm_page.h"

#ifdef __cplusplus
extern "C" {
#endif

// 类型声明
struct tm_list_popup_vars {
    tm_anim win_x;      // 弹窗的横坐标
    tm_anim list_y;     // 列表y值动画
    tm_slide_str str_ss;    // 滑动字符串
    uint8_t line_n;     // = DISP_H / line_h; 屏幕内有多少行选
    uint8_t line_h;     // 行高：文字行高，或图标高度加上下边距
    int16_t ind_y_temp; // 指示器的y坐标暂存
};
// A menu in a popup: the first fields are tm_menu's, as in every menu page. A row
// shows the option's icon (with an icon size) or its own mark, if it sets one, and
// its text.
typedef struct {
    tm_page page;
    uint8_t item_num;         // 选项个数
    tm_option* option_array;  // 选项数组
    uint8_t select_item;      // 选中项
    uint8_t icon_w, icon_h;   // 图标大小，0 为不显示图标(在打开前设置)
    tm_option* bg_opt;           // 自动获取到的背景页面的选项的指针
    bool auto_set_bg_opt : 1; // 弹窗页面是否自动设置lastPage中的select_opt相关的值
    struct tm_list_popup_vars vars; // 本页面的状态
} tm_list_popup;
/**
 * @brief 初始化列表弹窗
 * @param lw 列表弹窗指针
 * @param item_num 选项个数
 * @param option_array 选项数组(每个选项显示 text，有图标大小时还显示 icon)
 * @param auto_set_bg_opt 是否自动设置背景选项
 * @param cb 回调函数指针
 * @attention auto_set_bg_opt
 * 为true会在单击时把选中项的 text 设置到背景页面选中选项的 content 中
 */
void tm_list_popup_init(tm_list_popup* lw, tm_ui* ui, uint8_t item_num, tm_option* option_array,
                        bool auto_set_bg_opt, tm_page_cb cb);

/**
 * @brief 选中第 index 项(例如打开弹窗前选中当前的值)，列表直接滚动到能看到它的位置
 */
void tm_list_popup_select(tm_list_popup* lw, uint8_t index);

// Moves the selection by delta rows (wrapping with loop_param[TM_LOOP_LIST_POPUP]); false
// if it did not move.
bool tm_list_popup_move(tm_list_popup* lw, int16_t delta);

#ifdef __cplusplus
}
#endif

#endif
