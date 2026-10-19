#ifndef TM_CORE_TM_UI_H_
#define TM_CORE_TM_UI_H_

#include "tm_common.h"
#include "core/tm_anim.h"
#include "core/tm_msg.h"
#include "core/tm_page.h"
#include "gfx/tm_draw.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
版本更新日志：
Version-1.0.0[2025.02.09]:
1. 适配多尺寸屏幕，可以通过WouoUI_conf.h中的宽长的宏定义更改屏幕宽长，所有页面的元素都会自动居中
2. 字体自适应，同样在WouoUI_conf.h中的修改页面使用的字体，页面会自适应字体的宽高
3. 长文本自动滚动，长文本在选中时会自动滚动，同时滚动模式、速度和起始延时都有参数可以调整
4. 基于anim、slide动画监视的软件动态刷新，相比硬件buff对比动态刷新：
    - 软件刷新：基于动画监视，静止时完全停止状态机，只会轮询msg输入，通用性不如硬件动态刷新。
    -
硬件刷新：基于buff对比，通用性强，但需要多一个buff作为对比，同时静止时，内部buff的clear和重新写入一直在运行，只是没有将buff发送
5.
主状态机优化，将弹窗抽象为页面(弹窗状态机和页面状态机融合)，支持任意页面的弹窗调用和弹窗自身的嵌套调用
6.
修改了回调函数类型，回调函数会将该页面msg传入，方便使用者进行更灵活的开发，且所有页面回调函数调用机制统一————只要有该页面有msg输入就会调用
7. 宏参数检查和LOG提示
版本致谢：
@(bilibili叻叻菌)[https://space.bilibili.com/86391945] 为这个版本提供了
    - 指示器连贯的丝滑动画
    - 波形页面的优化
    - 用于浮点数调整的Digit弹窗
    - 使用渐进整形的动画参数代替浮点运算的神来之笔(👍)
Todo List：
[ ] 将中文接口完善，支持中文
[ ] 加入图片显示页面和钟表页面，完成宏定义裁剪，适配前一个版本内存小的air001😭
[ ] 实现一个C++版本，使其能作为arduino的库使用(正好在学C++🤣)
[ ] 实现在freetros中线程安全的版本，这个版本中对全局变量的分类和整理也是为了这个做准备😀
本项目地址和开发者：
@ (项目地址)[https://github.com/Sheep118/tm_ui-PageVersion]
@ (本人bilibili)[https://space.bilibili.com/679703519]
@ (叻叻菌bilibili)[https://space.bilibili.com/86391945]
*/




/*============================================常量定义=========================================*/
// 定义字符串类型
typedef enum tm_ui_state {
    TM_STATE_OUT = 0x00, // ui层级退出时
    TM_STATE_IN,         // ui层级深入时
    TM_STATE_PROC,       // ui页面处理时
    TM_STATE_SLIDE,      // 全页面侧滑切换：旧页面滑出，新页面同时滑入
} tm_ui_state;              // UI状态机
//------类别下标声明。用于UI参数数组中做索引
enum tm_ani_kind // 动画速度类别(数组中的下标)
{
    TM_ANI_INDICATOR = 0x00, // 指示器动画速度
    TM_ANI_TILE,       // 磁贴动画速度
    TM_ANI_LIST,       // 列表动画速度
    TM_ANI_WAVE,       // 波形动画速度
    TM_ANI_POPUP,        // 弹窗动画速度
    TM_ANI_FADE,       // 页面渐变(模糊)退出速度
    TM_ANI_SLIDE,      // 页面侧滑切换速度
    TM_ANI_COUNT,    // 动画速度参数的数目，用于数组初始化
};
enum tm_unfold_kind // 展开方式类别(数组中的下标)
{
    TM_UNFOLD_TILE = 0x00, // 磁贴图标从头展开开关
    TM_UNFOLD_LIST,        // 菜单列表从头展开开关
    TM_UNFOLD_COUNT,     // 展开方式类别数目
};
enum tm_loop_kind // 循环模式类别(数组中的下标)
{
    TM_LOOP_TILE = 0x00, // 磁贴图标循环模式开关
    TM_LOOP_LIST,        // 菜单列表循环模式开关
    TM_LOOP_LIST_POPUP,    // 列表弹窗循环开关
    TM_LOOP_COUNT,     // 循环模式类别数目
};
enum tm_slide_step_kind {
    TM_SLIDE_STEP_TILE = 0x00, // 磁贴文本滚动步长
    TM_SLIDE_STEP_LIST_TEXT,   // 列表文本滚动步长
    TM_SLIDE_STEP_LIST_VALUE,    // 列表值滚动步长
    TM_SLIDE_STEP_WAVE_TEXT,   // 波形页面文本滚动步长
    TM_SLIDE_STEP_WAVE_VALUE,    // 波形页面数值滚动步长
    TM_SLIDE_STEP_POPUP_TEXT,     // 弹窗内文本的滚动步长
    TM_SLIDE_STEP_POPUP_VALUE,     // 弹窗内数值的滚动步长
    TM_SLIDE_STEP_COUNT,
};
enum tm_popup_blur_kind {
    TM_POPUP_BLUR_MSG = 0x00, // msgwin背景模糊程度下标
    TM_POPUP_BLUR_CONFIRM,       // msgwin背景模糊程度下标
    TM_POPUP_BLUR_VALUE,        // msgwin背景模糊程度下标
    TM_POPUP_BLUR_DIGIT,       // msgwin背景模糊程度下标
    TM_POPUP_BLUR_LIST,       // msgwin背景模糊程度下标
    TM_POPUP_BLUR_TEXT_INPUT,
    TM_POPUP_BLUR_PROGRESS,
    TM_POPUP_BLUR_COUNT,
};
// How the selected row or button is marked.
// How every menu marks its selection: list rows, list popups, buttons, tiles and grid icons.
typedef enum tm_highlight {
    TM_HIGHLIGHT_FILL = 0, // 背景反色：反色实心框，框内文字反白
    TM_HIGHLIGHT_FRAME,    // 边框：空心框
    TM_HIGHLIGHT_CORNERS,  // 四角：只画四个直角
} tm_highlight;
// How wide the highlight of a list row is.
typedef enum tm_highlight_width {
    TM_HIGHLIGHT_FIT_TEXT = 0, // 跟随选中项文字的长度
    TM_HIGHLIGHT_FULL_ROW,     // 整行(到滚动条为止)
} tm_highlight_width;
// How one full page replaces another (popups always drop in over the page).
typedef enum tm_transition {
    TM_TRANSITION_FADE = 0, // 旧页面渐隐，新页面渐显
    TM_TRANSITION_SLIDE,    // 侧滑：进入下级从右边推入，返回时从左边推入
    TM_TRANSITION_CASCADE,  // 旧页面渐隐后，列表的行从上到下依次滑入(其他页面照常进入)
} tm_transition;
// Look of a UI; set ui->params.style after tm_ui_init().
typedef struct tm_style {
    bool animate;     // false: 指示器、页面和弹窗直接到位，页面切换不做渐隐(文字滚动不受影响)
    uint8_t radius;   // 圆角半径上限：0 全部直角，默认 255 保留各元素自己的倒角
    tm_highlight highlight;             // 所有菜单的选中样式：背景反色、边框或四角
    tm_highlight_width highlight_width; // 列表和列表弹窗的选中框宽度
    tm_transition transition;           // 全页面切换方式(animate 为 false 时直接切换)
} tm_style;

typedef struct tm_ui_params {
    tm_style style;                                // 渲染样式
    uint16_t ani_param[TM_ANI_COUNT];              // 动画参数数组
    uint8_t ufd_param[TM_UNFOLD_COUNT];               // 展开参数数组
    uint8_t loop_param[TM_LOOP_COUNT];             // 循环参数数组
    uint8_t slidestrstep_param[TM_SLIDE_STEP_COUNT];      // 文本滚动步长参数数组
    tm_slide_mode slidestrmode_param[TM_SLIDE_STEP_COUNT]; // 文本滚动步长参数数组
    tm_blur winbgblur_param[TM_POPUP_BLUR_COUNT];     // 背景模糊参数的数组
} tm_ui_params;                                         // UI参数集合类型
// Defaults copied into every UI by tm_ui_init(); change ui->params afterwards to tune one UI.
extern const tm_ui_params tm_default_params;

// Two complete looks to start from.
typedef enum tm_preset {
    TM_PRESET_DEFAULT = 0, // tm_default_params: animated, rows cascade in, whole-row inverted selection
    TM_PRESET_CLASSIC,     // a plain menu: no motion, square corners, whole row inverted,
                           // no popup dimming
} tm_preset;
// Replaces ui->params with a preset.
void tm_ui_set_preset(tm_ui* ui, tm_preset preset);

// 指示器
typedef struct tm_indicator {
    tm_anim x;
    tm_anim y;
    tm_anim w;
    tm_anim h;
} tm_indicator;

typedef struct tm_scrollbar {
    bool display; // 是否显示滚动条
    tm_anim y;    // 滚动条y坐标
} tm_scrollbar;

typedef struct tm_ui_blur {
    uint8_t blur_cur : 3;
    uint8_t blur_tgt : 3;
    bool blur_end;
    uint16_t timer; // 用于演示虚化函数，
    uint16_t blur_time;
} tm_ui_blur;

// WouoUI类型，整个UI类型
/**
 * @brief 把帧缓冲交给显示驱动
 * @param fb 帧缓冲(页优先：像素 (x, y) 是 buf[(y / 8) * w + x] 的第 y % 8 位)
 * @param first_page 第一个有变化的页(8 行一页)
 * @param end_page 最后一个有变化的页之后的页，只需要传输 [first_page, end_page) 这些页
 * @param user tm_ui_init 时传入的用户指针
 */
typedef void (*tm_flush_fn)(const tm_fb* fb, int16_t first_page, int16_t end_page, void* user);

typedef struct tm_ui {
    uint8_t is_motionless : 1; // 静止时不再绘制和刷屏，有消息或数据输入时恢复
    uint8_t flush_all : 1;     // 下一次刷屏传输所有页
    uint8_t slide_is_finish : 1; // 滚动动画是否结束
    uint8_t anim_is_finish;      // 用于查看非线性动画是否结束的标志位
    tm_page_addr home_page;          // 主页面的地址
    tm_page_addr current_page;       // 当前页面的地址
    tm_page_addr in_page; // 记录fade out后要进入的页面地址(在jump中暂存下页面，之后才能给cur_page)
    tm_fb fb;                    // 帧缓冲(存储由调用者提供)
    tm_flush_fn flush;           // 刷新屏幕的函数
    void* flush_user;            // 传给 flush 的用户指针
    uint32_t page_hash[TM_MAX_PAGES]; // 上次刷屏时每页内容的哈希，用于找出变化的页
    tm_canvas w_all;             // 全局画布变量，所有的绘制都在这个窗口内进行
    tm_ui_state state;                    // ui状态变量
    tm_ui_params params;         // 动画、展开、循环、滚动、虚化参数
    tm_msg_queue msg_queue;            // 消息队列
    tm_indicator indicator;              // 指示器
    tm_scrollbar scrollBar;              // 滚动条
    tm_ui_blur ui_blur;                   // UI模糊
    tm_blur win_bg_blur;          // 弹窗存在时背景模糊的程度
    uint16_t time;               // 本帧经过的时间(毫秒)，由 tm_ui_proc 设置
    // 侧滑切换：滑动距离(0 到屏幕宽)，方向(1 进入下级，-1 返回)，以及旧页面离开时的指示器和滚动条
    tm_anim slide;
    int8_t slide_dir;
    bool slide_settled; // 新页面的进入动画已完成，之后用 show 绘制
    tm_indicator slide_indicator;
    tm_scrollbar slide_bar;
} tm_ui;

//============================================全局变量的外界声明================================
/**
 * @brief 初始化一个UI实例，之后再初始化它的页面(第一个初始化的页面是主页面)
 * @param buf 帧缓冲的存储，tm_fb_size(w, h) 字节，由调用者持有
 * @param w 屏幕宽度(像素)
 * @param h 屏幕高度(像素)
 * @param flush 把帧缓冲中变化的部分交给显示驱动的函数
 * @param user 原样传给 flush
 */
void tm_ui_init(tm_ui* ui, uint8_t* buf, int16_t w, int16_t h, tm_flush_fn flush, void* user);

/**
 * @brief 把一个输入消息放进UI的队列(队列满时丢弃)，在下一次 tm_ui_proc 中处理
 */
void tm_ui_send(tm_ui* ui, tm_msg msg);

/**
 * @brief 处理一帧：响应输入、推进动画、需要时绘制并刷屏
 * @param elapsed_ms 距上一次调用经过的真实时间(毫秒)。动画按它推进，所以帧率高低不影响动画快慢。
 */
void tm_ui_proc(tm_ui* ui, uint16_t elapsed_ms);

/**
 * @brief 让下一次 tm_ui_proc 重新绘制并整屏刷新(例如显示器断电重启、内容丢失之后)
 */
void tm_ui_invalidate(tm_ui* ui);

/**
 * @brief 当前显示的页面
 */
tm_page* tm_ui_current_page(const tm_ui* ui);

/**
 * @brief 从 self_page 跳转到 terminate_page(两者属于同一个UI)，并记住返回路径
 */
void tm_jump_to_page(tm_page_addr self_page, tm_page_addr terminate_page);

// Duration of an animation of this kind, 0 when the style turns animation off.
uint16_t tm_ui_anim_ms(const tm_ui* ui, enum tm_ani_kind kind);
// A corner radius limited by the style.
int16_t tm_ui_radius(const tm_ui* ui, int16_t r);
// Draws the selection highlight at the indicator's current position in the style's look.
void tm_ui_draw_highlight(tm_ui* ui, int16_t r);

/**
 * @brief 获取选项的浮点值
 */
float tm_option_float(tm_option* option);

#ifdef __cplusplus
}
#endif

#endif
