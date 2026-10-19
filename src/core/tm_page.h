#ifndef TM_CORE_TM_PAGE_H_
#define TM_CORE_TM_PAGE_H_

#include "tm_common.h"
#include "core/tm_anim.h"
#include "core/tm_format.h"
#include "core/tm_msg.h"
#include "gfx/tm_draw.h"
#include "gfx/tm_text.h"

#ifdef __cplusplus
extern "C" {
#endif

//===================================页面共用的类型参数方法==================================
// 定义字符串类型
//--------页面类型枚举
typedef enum {
    TM_PAGE_NONE =
        0x00, // 非页面类型,用于在jump函数判断一个页面是否经过初始化,如果没有,则没有绑定过方法,避免状态机访问方法报错
    TM_PAGE_TILE,       // 磁贴类
    TM_PAGE_LIST,        // 列表类
    TM_PAGE_WAVE,        // 波形类
    TM_PAGE_GRID,        // 图标矩阵
    TM_PAGE_CHART,       // 图表(柱状图、横条、折线)
    TM_PAGE_POPUPS,         // 类型分割线，以上是全页面类型，以下是弹窗页面类型
    TM_PAGE_VALUE_POPUP, // 滑动数值弹窗
    TM_PAGE_MSG_POPUP,      // 消息弹窗
    TM_PAGE_CONFIRM_POPUP,     // 确认弹窗
    TM_PAGE_DIGIT_POPUP,     // 微调数值弹窗
    TM_PAGE_TEXT_INPUT_POPUP, // ASCII 文本输入弹窗
    TM_PAGE_PROGRESS_POPUP,   // 进度弹窗
    TM_PAGE_LIST_POPUP,     // 列表弹窗
} tm_page_type;           // 页面类型，用于标志传入的每个页面类型，方便调用对应的proc函数
//-------页面成员类型
// What a list row shows after its text. Tile pages show the text only.
typedef enum {
    TM_OPTION_PLAIN = 0, // 只有文本
    TM_OPTION_INT,       // 行尾显示 val(整数)
    TM_OPTION_FIXED,     // 行尾显示 val，按 decimals 显示为定点数
    TM_OPTION_TEXT,      // 行尾显示 content
    TM_OPTION_SWITCH,    // 行尾显示选框，单击时列表自动翻转 val
    TM_OPTION_CHECK,     // 行尾显示选框，val 由应用(例如确认弹窗)设置
} tm_option_kind;

typedef struct tm_option {
    const char* text;      // 显示的文本
    tm_option_kind kind;   // 行尾显示什么
    char mark;             // 列表里文本前的标记字符，0 表示按 kind 取默认值(见 tm_option_mark)
    const uint8_t* icon;   // 图标(30x30，tm_icon)；图标列表、磁贴和矩阵显示它，没有时显示 mark
    const char* content;         // TM_OPTION_TEXT 的行尾文本
    int32_t val;           // 选项的值
    tm_decimals decimals;  // TM_OPTION_FIXED 的小数位数
    uint8_t order;         // 该选项在列表/磁贴中的序号，由页面初始化时设置
} tm_option;

// The mark shown before the option's text in a list: option->mark, or one for its kind.
char tm_option_mark(const tm_option* option);
// 选择类界面的回调函数类型定义,参数为确认选中项的指针。
typedef struct tm_page tm_page; // 页面基类的声明,方便作为回调函数参数
typedef struct tm_ui tm_ui;     // 页面所属的UI
// 更改回调函数直接传入当前页面的页面地址
// Draws what the application adds to a page, after the page itself and before the
// selection highlight: a status bar, a caption with the selected item's name, a reading.
// The canvas is the whole screen; while the page slides in or out it moves with it.
typedef void (*tm_page_draw_cb)(const tm_page* page, tm_canvas* canvas);
typedef bool (*tm_page_cb)(
    const tm_page* cur_page_addr,
    tm_msg msg); // 页面回调函数类型定义(会传入当前页面的地址和当前页面收到的操作消息)
// 页面地址类型，用于声明全局页面数组存放页面使用
typedef void *tm_page_addr;
// 每个页面基本方法的定义
typedef bool (*tm_page_in_fn)(
    tm_page_addr); // 进入页面的过度动画函数(返回值为True表示这个状态已经可以结束了，切换下一个状态)
typedef void (*tm_page_in_init_fn)(tm_page_addr); // 进入页面过度动画参数初始化函数
typedef void (*tm_page_show_fn)(tm_page_addr);       // 页面的展示函数
typedef bool (*tm_page_react_fn)(
    tm_page_addr); // 页面的响应函数(返回值为True表示这个状态已经可以结束了，切换下一个状态)
typedef void (*tm_page_indicator_fn)(tm_page_addr); // 页面与UI指示器交互的控制函数
typedef void (*tm_page_scrollbar_fn)(tm_page_addr); // 页面与UI滚动条交互的控制函数
typedef struct {
    tm_page_in_fn in;                        // 进入页面的过度动画函数
    tm_page_in_init_fn in_para_init;      // 进入页面过度动画参数初始化函数
    tm_page_show_fn show;                    // 页面的展示函数
    tm_page_react_fn react;                  // 页面的响应函数
    tm_page_indicator_fn indicator_ctrl; // 页面与UI指示器交互的控制函数
    tm_page_scrollbar_fn scrollbar_ctrl; // 页面与UI滚动条交互的控制函数
} tm_page_methods;                        // 方法集合
struct tm_page {
    tm_page_type page_type;   // 页面类型，以便在处理时调用不同函数绘制
    tm_ui* ui;                // 所属的UI
    tm_page_addr last_page;   // 上级页面的地址
    tm_page_cb cb;      // 页面的回调函数
    const tm_page_methods* methods; // 页面方法集合的指针，其实例在UI对象上
    tm_page_draw_cb draw; // 页面画完自己之后由应用再画的内容(可为 NULL)，见 tm_page_set_draw
    bool
        auto_input; // 页面是否会自动处理消息,默认会,如果置False,需要自己在回调函数中调用提供的接口进行页面操作
}; // 最基本的页面类型(所有页面类型的基类和结构体的**第一个成员**)

// What every menu page (list, tile, grid) starts with, in this order, so code that only
// needs the items and the selection works with any of them.
typedef struct tm_menu {
    tm_page page;
    uint8_t item_num;
    tm_option* option_array; // 每一项画 mark 或 icon，由页面的显示方式决定
    uint8_t select_item;
    uint8_t icon_w, icon_h;  // 选项图标的大小(option.icon 都是这个大小)；列表为 0 时显示 mark
} tm_menu;
// Sets what the application draws on top of a page; NULL for nothing.
void tm_page_set_draw(tm_page_addr page, tm_page_draw_cb draw);
// Draws a page with what the application adds to it: its show, then its draw callback.
// Popups draw the page under them with it.
void tm_page_show(tm_page_addr page);
// The same for a page's entry animation; returns what its in method returns.
bool tm_page_in(tm_page_addr page);

// Whether a page is a menu (list, tile, grid, chart or list popup): it starts like tm_menu.
bool tm_page_is_menu(const void* page);
// The selected option of any menu page, or NULL for a page that is not a menu.
tm_option* tm_menu_selected(const void* page);
// 页面的通用方法

/**
 * @brief 页面类型及相关检查宏定义
 *
 * @def tm_page_type_of(page_addr)
 * @brief 获取指定页面的类型
 * @param page_addr 页面地址指针
 * @return 返回页面类型
 */
#define tm_page_type_of(page_addr) (((tm_page*)page_addr)->page_type)
/**
 * @def tm_page_is_popup(page_addr)
 * @brief 检查指定页面是否为窗口类型
 * @param page_addr 页面地址指针
 * @return 如果是窗口类型返回true，否则返回false
 */
#define tm_page_is_popup(page_addr) ((uint8_t)tm_page_type_of(page_addr) > TM_PAGE_POPUPS)
/**
 * @def tm_page_is_inited(page_addr)
 * @brief 检查指定页面是否已初始化
 * @param page_addr 页面地址指针
 * @return 如果已初始化返回true，否则返回false
 */
#define tm_page_is_inited(page_addr) (TM_PAGE_NONE != tm_page_type_of(page_addr))
/**
 * @brief 返回到指定页面
 * @param page_addr 目标页面地址
 * @attention 这个函数用于二次开发
 */
void tm_page_return(tm_page_addr page_addr);
/**
 * @brief 初始化指定页面
 * @param page_addr 页面地址
 * @param call_back 页面回调函数
 * @attention 这个函数用于二次开发
 */
void tm_page_init(tm_page_addr page_addr, tm_ui* ui, tm_page_cb call_back);

// Method stubs for page types that do not need a method.
void tm_func_do_nothing(tm_page_addr page_addr);
bool tm_func_do_nothing_ret_true(tm_page_addr page_addr);
bool tm_func_do_nothing_ret_false(tm_page_addr page_addr);
/**
 * @brief 设置页面是否自动处理消息(默认开启)
 * @attention 需要注意，如果开启自动处理，大多数弹窗页面的click都是默认返回的(数位和文字输入弹窗除外)，
 *            如果需要click事件跳转的话，需要失能自动消息处理，并自己在回调中调用对应的处理函数
 * @param page 目标页面结构体指针
 * @param open true开启自动处理,false关闭自动处理
 */
void tm_page_set_auto_input(tm_page* page, bool open);

#ifdef __cplusplus
}
#endif

#endif
