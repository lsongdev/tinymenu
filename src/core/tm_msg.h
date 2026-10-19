#ifndef TM_CORE_TM_MSG_H_
#define TM_CORE_TM_MSG_H_

#include "tm_common.h"

#ifdef __cplusplus
extern "C" {
#endif

//---------输入消息枚举类型
typedef enum {
    // Directions as on the screen, from a d-pad. Where a page gives a direction no
    // meaning of its own, right and up go forward, left and down back, as for a value.
    TM_MSG_UP = 0x00,
    TM_MSG_DOWN,
    TM_MSG_LEFT,
    TM_MSG_RIGHT,
    TM_MSG_CLICK,     // 点击消息，表确认，确认某一选项，回调用一次回调
    TM_MSG_RETURN,    // 返回消息，表示返回，从一个页面退出
    TM_MSG_HOME, // home消息，表回主界面(尚未设计，目前还没有设计对应的功能，默认以page_id为0的页面为主页面)
    // One axis, for two or three buttons and encoders: back and forth through the
    // page in reading order, and down/up a value. Every page works with these alone.
    TM_MSG_PREV,
    TM_MSG_NEXT,
    TM_MSG_NONE = 0xFF, // none表示没有操作
} tm_msg;          // 输入消息类型，UI设计只供输入5种消息
// 输入消息缓冲队列的大小(至少为2)
typedef struct {
    tm_msg queue[TM_MSG_QUEUE_SIZE]; // 输入消息队列存放数组
    uint8_t font;                         // 队列头指针
    uint8_t rear;                         // 队列尾指针
} tm_msg_queue;

//--------msg_que处理相关函数
/**
 * @brief 判断消息是否为方向消息
 *
 * @param msg 输入的消息
 * @return 如果消息为方向消息，返回true，否则返回false
 */
#define tm_msg_is_direct_msg(msg)                                                              \
    ((uint8_t)(msg) < (uint8_t)TM_MSG_CLICK || (msg) == TM_MSG_PREV || (msg) == TM_MSG_NEXT)

// Step through a vertical sequence (a list) in reading order: -1 for prev, up and
// left, +1 for next, down and right, 0 otherwise.
static inline int8_t tm_msg_step(tm_msg msg) {
    return msg == TM_MSG_NEXT || msg == TM_MSG_DOWN || msg == TM_MSG_RIGHT  ? 1
           : msg == TM_MSG_PREV || msg == TM_MSG_UP || msg == TM_MSG_LEFT ? -1
                                                                          : 0;
}

// Change a value, or step through a horizontal sequence (tiles, waves): +1 for next,
// up and right, -1 for prev, down and left, 0 otherwise.
static inline int8_t tm_msg_amount(tm_msg msg) {
    return msg == TM_MSG_NEXT || msg == TM_MSG_UP || msg == TM_MSG_RIGHT  ? 1
           : msg == TM_MSG_PREV || msg == TM_MSG_DOWN || msg == TM_MSG_LEFT ? -1
                                                                            : 0;
}

/**
 * @brief 判断消息队列是否为空
 *
 * @param msg_que 输入的消息队列指针
 * @return 如果消息队列为空，返回true，否则返回false
 */
bool tm_msg_que_is_empty(tm_msg_queue* msg_que);

/**
 * @brief 判断消息队列是否已满
 *
 * @param msg_que 输入的消息队列指针
 * @return 如果消息队列已满，返回true，否则返回false
 */
bool tm_msg_que_is_full(tm_msg_queue* msg_que);

/**
 * @brief 发送消息到消息队列
 *
 * @param msg_que 输入的消息队列指针
 * @param msg 要发送的消息
 * @attention 如果消息队列已满，消息将不会被发送
 */
void tm_msg_que_send(tm_msg_queue* msg_que, tm_msg msg);

/**
 * @brief 从消息队列读取消息
 *
 * @param msg_que 输入的消息队列指针
 * @return 读取到的消息
 * @attention 如果消息队列为空，返回的消息可能无效
 */
tm_msg tm_msg_que_read(tm_msg_queue* msg_que);

/**
 * @brief 清空消息队列
 *
 * @param msg_que 输入的消息队列指针
 * @return 如果清空成功，返回true，否则返回false
 */
bool tm_msg_que_clear(tm_msg_queue* msg_que);

#ifdef __cplusplus
}
#endif

#endif
