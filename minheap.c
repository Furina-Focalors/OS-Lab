//
// Created by Furina on 2023/12/28.
//
#include "minheap.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"

// 初始化堆
void initializeHeap(struct MinHeap* heap) {
    heap->size = 0;
    heap->weight_sum = 0;
    initlock(&(heap->lock), "sched_queue");
}

// 插入元素
void insert(struct MinHeap* heap, struct proc *p) {
    while(heap->size >= MAX_HEAP_SIZE);// 忙等待

    acquire(&(heap->lock));
    heap->processes[heap->size] = p;
    heapifyUp(heap, heap->size);
    heap->size++;
    heap->weight_sum += sched_prio_to_weight[p->nice + NICE_OFFSET];
    release(&(heap->lock));
}

// 删除最小元素
struct proc *deleteMin(struct MinHeap* heap) {
    if (heap->size <= 0) {
        return 0;
    }

    acquire(&(heap->lock));
    struct proc *min = heap->processes[0];
    if(heap->size==1){
        heap->processes[--(heap->size)] = 0;
        release(&(heap->lock));
        return min;
    }
    heap->processes[0] = heap->processes[heap->size - 1];
    heap->processes[heap->size - 1] = 0;
    heap->size--;
    heapifyDown(heap, 0);
    heap->weight_sum -= sched_prio_to_weight[min->nice + NICE_OFFSET];
    release(&(heap->lock));

    return min;
}

// 从给定索引开始向上调整堆
void heapifyUp(struct MinHeap* heap, int index) {
    while (index > 0) {
        int parentIndex = (index - 1) / 2;
        if (heap->processes[index]->vruntime < heap->processes[parentIndex]->vruntime) {
            // 交换当前节点和父节点
            struct proc *temp = heap->processes[index];
            heap->processes[index] = heap->processes[parentIndex];
            heap->processes[parentIndex] = temp;

            // 更新索引，继续向上调整
            index = parentIndex;
        } else {
            break; // 堆性质已满足，退出循环
        }
    }
}

// 从给定索引开始向下调整堆
void heapifyDown(struct MinHeap* heap, int index) {
    int leftChild = 2 * index + 1;
    int rightChild = 2 * index + 2;
    int smallest = index;

    // 找到左右子节点中最小的节点
    if (leftChild < heap->size && heap->processes[leftChild]->vruntime < heap->processes[smallest]->vruntime) {
        smallest = leftChild;
    }
    if (rightChild < heap->size && heap->processes[rightChild]->vruntime < heap->processes[smallest]->vruntime) {
        smallest = rightChild;
    }

    if (smallest != index) {
        // 交换当前节点和最小节点
        struct proc *temp = heap->processes[index];
        heap->processes[index] = heap->processes[smallest];
        heap->processes[smallest] = temp;

        // 更新索引，继续向下调整
        heapifyDown(heap, smallest);
    }
}
