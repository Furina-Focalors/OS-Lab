//
// Created by Furina on 2023/12/4.
//

#include "defs.h"
#include "msg.h"
#include "mmu.h"
#include "spinlock.h"

// 任意类型消息
#define ALL_TYPE 0
#define MAX_MSG_COUNT (PGSIZE - sizeof(uint) - sizeof(struct spinlock)) / sizeof(struct msg*)

//struct msg_queue{
//    int id;// 消息队列的id
//    struct spinlock lock;// 向消息队列的写入必须是同步的
//    int size;// 消息队列的内存空间占用
//    struct msg* first;// 消息队列的头节点
//};
// 消息队列
struct msg_queue{
    uint id;// 消息队列的id
    struct spinlock lock;// 向消息队列的写入必须是同步的
    struct msg *messages[MAX_MSG_COUNT];// 存放消息的空间，index=0处默认存放头节点first
};

#define MAX_MSQ_COUNT PGSIZE / sizeof(struct msg_queue)

// 消息队列指针表
struct msg_queue *msq_table[MAX_MSQ_COUNT];
struct spinlock msq_table_lock;

/**
 * 寻找给定消息队列中第一个为空的消息槽
 * @param msg_queue *mq: 消息队列的指针
 * @return 查找成功返回消息槽的下标，失败返回-1
 */
int find_msq_slot(struct msg_queue *mq){
    for(int i=1;i<MAX_MSG_COUNT;++i){// 下标0为头节点
        if(!mq->messages[i])return i;
    }
    return -1;
}

/**
 * 查找消息列表中指定id的消息队列
 * @param uint id: 需要查找的消息队列id
 * @return 查找成功返回对应消息队列的下标，否则返回-1
 */
int find_msq(uint id){
    for(int i=0;i<MAX_MSQ_COUNT;++i){
        if(msq_table[i] && msq_table[i]->id==id)return i;
    }
    return -1;
}

/**
 * 查找消息队列表中的 第一个 空闲槽
 * @return 查找成功返回对应下标，若表已满，返回-1
 */
int find_table_slot(){
    for(int i=0;i<MAX_MSQ_COUNT;++i){
        if(!msq_table[i])return i;
    }
    return -1;
}

/**
 * 查找指定队列中指定类型的 第一条消息
 * @param msg_queue msq: 消息队列的指针
 * @param long msg_type: 需要接收的消息类型，ALL_TYPE（0）表示任意类型
 * @param msg** msg: 存放结果指针
 */
void find_msg(struct msg_queue *msq, long msg_type, struct msg **msg){
    msg = &msq->messages[0];
    while((*msg)->next) {
        *msg = (*msg)->next;
        if(!msg_type || (*msg && (*msg)->msg_type == msg_type)) {
            return;
        }
    }
    *msg = 0;
    return;
}

/**
 * 创建消息队列
 * @param uint id: 消息队列的id
 * @return 创建成功返回0，否则返回-1
 */
int create_msg_queue(uint id){
    acquire(&msq_table_lock);

    if(find_msq(id) != -1) return -1;
    // 寻找消息队列表中一个空闲的项
    int index = find_table_slot();
    if(index == -1) return -1;

    // 分配内核空间
    struct msg_queue *msq = (struct msg_queue*)kalloc();
    if(!msq){
        panic("create_msg_queue: failed to allocate memory.");
    }

    // init
    msq->id = id;
    initlock(&msq->lock, "msq");
    // first
    msq->messages[0] = (struct msg*)kalloc();
    if(!msq->messages[0]){
        panic("create_msg_queue: failed to allocate memory.");
    }
    msq->messages[0]->data = 0;
    msq->messages[0]->next = 0;
    msq->messages[0]->msg_type = 0;
    for(int i=1;i<MAX_MSG_COUNT;++i){
        msq->messages[i] = 0;
    }
    // 将消息队列记录到表中
    msq_table[index] = msq;

    release(&msq_table_lock);
    return 0;
}

/**
 * 删除消息队列
 * @param uint id: 消息队列的id
 * @return 删除成功返回0，否则返回-1
 */
int delete_msg_queue(uint id){
    acquire(&msq_table_lock);

    int msq_index = find_msq(id);
    if(msq_index == -1) return -1;
    // 删除队列中的所有消息
    struct msg *cur = msq_table[msq_index]->messages[0];
    while(cur){
        struct msg *temp = cur;
        cur = cur->next;
        kfree((char*)temp);
    }
    // 删除相应的消息队列
    kfree((char*)msq_table[msq_index]);// causes panic if failed
    msq_table[msq_index] = 0;

    release(&msq_table_lock);
    return 0;
}

/**
 * 发送消息
 * @param uint msq_id:      消息队列的id
 * @param const_void *msg:  指向用户空间消息的指针
 * @param long msg_type:    消息的类型，非0
 * @return 发送成功返回0，否则返回-1
 */
int msg_send(uint msq_id, const void *msg, long msg_type){
    if(!msg_type) return -1;
    // 查找消息队列表
    int msq_index = find_msq(msq_id);
    if(msq_index == -1)return -1;

    acquire(&msq_table[msq_index]->lock);

    // 查找空闲的消息槽
    int msg_index = find_msq_slot(msq_table[msq_index]);
    if(msg_index == -1)return -1;
    // 寻找插入位置
    struct msg *cur = msq_table[msq_index]->messages[0];
    while(cur->next){
        cur = cur->next;
    }
    // 向相应队列中添加消息
    msq_table[msq_index]->messages[msg_index] = (struct msg*)kalloc();
    if(!msq_table[msq_index]->messages[msg_index]) panic("msg_send: failed to allocate memory.");
//    msq_table[msq_index]->messages[msg_index]->data = (void*)kalloc();
//    if(!msq_table[msq_index]->messages[msg_index]) panic("msg_send: failed to allocate memory.");
//    *(char*)msq_table[msq_index]->messages[msg_index]->data = *(char*)msg;
    msq_table[msq_index]->messages[msg_index]->data = (void*)msg;
    msq_table[msq_index]->messages[msg_index]->msg_type = msg_type;
    msq_table[msq_index]->messages[msg_index]->next = 0;
    msq_table[msq_index]->messages[msg_index]->pre = cur;
    cur->next = msq_table[msq_index]->messages[msg_index];

    release(&msq_table[msq_index]->lock);
    return 0;
}

/**
 * 接收一条指定类型的消息
 * @param uint msq_id: 消息队列的id
 * @param void **msg:  消息的指针
 * @param long msg_type: 需要接收的消息类型，可以用ALL_TYPE（0）表示任意类型
 * @return void*: 若存在符合条件的消息，返回0，否则返回-1
 */
int msg_receive(uint msq_id, void **msg, long msg_type){
    // 查找消息队列表
    int msq_index = find_msq(msq_id);
    if(msq_index == -1)return -1;

    acquire(&msq_table[msq_index]->lock);

    // 查找消息
    struct msg** m = 0;
    find_msg(msq_table[msq_index], msg_type, m);
    // 仅复制消息的内容
    *msg = kalloc();
    if(!*msg) panic("msg_receive: failed to allocate memory.");
    *msg = (*m)->data;
    // 删除消息指针
    (*m)->pre->next = (*m)->next;
    (*m)->next->pre = (*m)->pre;
    kfree((char*)*m);
    *m = 0;

    release(&msq_table[msq_index]->lock);
    return -!*msg;
}