//
// Created by Furina on 2023/12/4.
//

#ifndef MSG_H
#define MSG_H

#include "types.h"

// 消息体
struct msg{
    long msg_type;// 消息的类型
    //uint msg_size;// 消息的长度
    void *data;// 消息的内容
    struct msg *next;// 指向下一个消息
    struct msg *pre;// 指向上一个消息
};

#endif