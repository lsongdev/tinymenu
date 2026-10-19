#include "core/tm_msg.h"

bool tm_msg_que_is_empty(tm_msg_queue* msg_que) { // 队列空
    return msg_que->font == msg_que->rear;
}

bool tm_msg_que_is_full(tm_msg_queue* msg_que) { // 队列满
    return (msg_que->rear + 1) % TM_MSG_QUEUE_SIZE == msg_que->font;
}

/**
 * @brief 向msg队列发送消息
 *
 * @param msg 消息
 */
void tm_msg_que_send(tm_msg_queue* msg_que, tm_msg msg) {
    if (!tm_msg_que_is_full(msg_que)) {
        msg_que->queue[msg_que->rear] = msg;
        msg_que->rear++;
        if (msg_que->rear == TM_MSG_QUEUE_SIZE)
            msg_que->rear = 0;
    }
}

/**
 * @brief 读msg队列消息
 *
 * @return tm_msg 消息
 */
tm_msg tm_msg_que_read(tm_msg_queue* msg_que) {
    tm_msg msg = TM_MSG_NONE;
    if (!tm_msg_que_is_empty(msg_que)) {
        msg = msg_que->queue[msg_que->font];
        msg_que->font++;
        if (msg_que->font == TM_MSG_QUEUE_SIZE)
            msg_que->font = 0;
    }
    return msg;
}

/**
 * @brief 清空msg队列消息
 */
bool tm_msg_que_clear(tm_msg_queue* msg_que) {
    msg_que->font = 0;
    msg_que->rear = 0;
    return tm_msg_que_is_empty(msg_que);
}
