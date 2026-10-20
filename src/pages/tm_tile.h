#ifndef TM_PAGES_TM_TILE_H_
#define TM_PAGES_TM_TILE_H_

#include "core/tm_page.h"
#include "pages/tm_icon_item.h"

#ifdef __cplusplus
extern "C" {
#endif

// TitlePage的类型定义
// tm_tile_page 需要的共用变量集合(类变量)
struct tm_tile_page_vars { // 因为Title选中项一直在中间，所以不需要有每个页面变量记录它真实页面的偏移位置
    tm_anim iconX;    // 图标的x坐标距选中目标的间距的变量
    tm_anim iconY;    // 图标的y坐标
    int16_t pitch;         // 相邻两项中心的距离(固定，不随名称加宽)
};
// The first fields are tm_icon_menu's, in the same order as in tm_grid_page.
typedef struct tm_tile_page { // 磁贴页面
    tm_page page;             // 基础页面信息
    uint8_t item_num;         // 页面选项个数
    tm_option* option_array;  // 选项数组，每项画它的 icon
    uint8_t select_item;      // 选中选项
    uint8_t icon_w, icon_h;   // 选项图标的大小(默认 30x30)
    tm_icon_name_mode name_mode; // 哪些图标下显示名称：全部(默认)、只有选中项或不显示
    tm_slide_str name_ss;        // 选中项的名称放不下时滚动
    uint8_t name_item;           // name_ss 属于哪一项；选中别的项时从头滚动
    struct tm_tile_page_vars vars; // 本页面的状态
} tm_tile_page;                               // 磁帖页面类型(所有类型页面，类型成员为第一个，方便查看)
// TitlePage的接口函数
/**
 * @brief 初始化磁贴页面
 * @param tile 磁贴页面结构体指针
 * @param item_num 选项数量(与 option_array 的大小一致；每个选项的 icon 字段给出它的图标)
 * @param option_array 选项数组指针
 * @param call_back 回调函数指针
 * @attention 数组大小需要使用者传入，确保传入的数组大小正确
 */
void tm_tile_page_init(tm_tile_page* tile, tm_ui* ui, uint8_t item_num, tm_option* option_array,
                          tm_page_cb call_back);
// Moves the selection by delta items (wrapping with loop_param[TM_LOOP_TILE]); false if it
// did not move.
bool tm_tile_page_move(tm_tile_page* tp, int16_t delta);
// Selects item index.
void tm_tile_page_select(tm_tile_page* tp, uint8_t index);

#ifdef __cplusplus
}
#endif

#endif
