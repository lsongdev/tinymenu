#ifndef TM_PAGES_TM_LIST_H_
#define TM_PAGES_TM_LIST_H_

#include "core/tm_page.h"

#ifdef __cplusplus
extern "C" {
#endif

// 检查check
// box的倒角是否符合大小(倒角太大会自动取到能取的最大值的，但还是提示一下)，这个检查在Init中执行，返回True需要打印log提示
// List页面类型定义
//  tm_list_page 需要的共用变量集合(类变量)
struct tm_list_page_vars {
    bool radio_click_flag; // 标志位，用于标记在单选项页面内是否单击了Click
    int16_t indicator_w_temp; // 用于暂时存储指示器长度，统一在ind_ctrl中赋值给指示器
    tm_anim optInt;           // 列表中每个选项的间隔值
    tm_slide_str opt_text_ss; // 滑动选项文字
    tm_slide_str opt_val_ss;  // 滑动选项数值
    uint16_t cascade_ms;      // 依次滑入：进入后经过的时间
    int16_t cascade_sel_x;    // 依次滑入：选中行当前的水平偏移(指示器跟着它)
    bool cascading;           // 正在依次滑入
};
typedef enum {
    TM_LIST_NORMAL = 0x00, // 普通列表
    TM_LIST_RADIO,         // 单选列表：单击一项会选中它的选框并清除其他选框
} tm_list_mode;
// ListPage页面类
// The first fields are tm_menu's, as in every menu page.
typedef struct tm_list_page {     // 列表页面
    tm_page page;                // 基础页面信息
    uint8_t item_num;         // 页面选项个数
    tm_option* option_array;     // 选项类型的数组(由于数组大小不确定，使用指针代替)
    uint8_t select_item;      // 选中选项
    uint8_t icon_w, icon_h;   // 图标大小：0 为文字列表(行首显示标记)，否则为图标列表
    tm_list_mode mode;        // 普通或单选
    uint8_t line_h;           // 行高：文字行高(13)或图标高度加上下边距，取较大的
    uint8_t line_n;           // = DISP_H / line_h; 屏幕内有多少行选
    int16_t ind_y_tgt;        // 存储指示器y目标坐标
    struct tm_list_page_vars vars; // 本页面的状态
} tm_list_page;                   // 列表页面类型(所有类型页面，类型成员为第一个，方便查看)
// ListPage的接口函数
/**
 * @brief 初始化列表页面
 * @param lp 列表页面指针
 * @param item_num 列表项目数量
 * @param option_array 选项数组指针
 * @param mode 普通或单选
 * @param call_back 回调函数
 * @note 用于创建和初始化一个新的列表页面
 */
void tm_list_page_init(tm_list_page* lp, tm_ui* ui, uint8_t item_num, tm_option* option_array,
                         tm_list_mode mode, tm_page_cb call_back);
// Moves the selection by delta rows (wrapping with loop_param[TM_LOOP_LIST]); false if it
// did not move.
bool tm_list_page_move(tm_list_page* lp, int16_t delta);
// Selects row index, scrolling to it.
void tm_list_page_select(tm_list_page* lp, uint8_t index);
/**
 * @brief 设置图标大小，在文字列表和图标列表之间切换
 * @param icon_w,icon_h 选项图标(option.icon)的大小；0 为文字列表，行首显示标记。
 *        图标列表里没有图标的行仍显示标记；行高随图标高度
 * @note 在页面显示前调用；会回到第一行
 */
void tm_list_page_set_icon_size(tm_list_page* lp, uint8_t icon_w, uint8_t icon_h);


#ifdef __cplusplus
}
#endif

#endif
