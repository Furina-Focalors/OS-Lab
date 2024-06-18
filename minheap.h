//
// Created by Furina on 2023/12/28.
//

#ifndef XV6_PUBLIC_MASTER_MINHEAP_H
#define XV6_PUBLIC_MASTER_MINHEAP_H

#include "defs.h"
#include "spinlock.h"

#define MAX_HEAP_SIZE 64

struct MinHeap {
    struct proc *processes[MAX_HEAP_SIZE];
    int size;
    struct spinlock lock;
    unsigned int weight_sum;
};

void initializeHeap(struct MinHeap* heap);
void insert(struct MinHeap* heap, struct proc *p);
struct proc *deleteMin(struct MinHeap* heap);
void heapifyUp(struct MinHeap* heap, int index);
void heapifyDown(struct MinHeap* heap, int index);

#endif //XV6_PUBLIC_MASTER_MINHEAP_H
