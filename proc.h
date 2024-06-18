#ifndef PROC_H
#define PROC_H

#include "minheap.h"

// Per-CPU state
struct cpu {
  uchar apicid;                // Local APIC ID
  struct context *scheduler;   // swtch() here to enter scheduler
  struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.关中断函数pushcli的嵌套调用次数
  int intena;                  // Were interrupts enabled before pushcli?值为1说明执行pushcli之前中断是启用的
  struct proc *proc;           // The process running on this cpu or null
  struct MinHeap proc_queue;   // 在该cpu上等待调度的进程队列
};

extern struct cpu cpus[NCPU];
extern int ncpu;

//PAGEBREAK: 17
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

# define NICE_0_LOAD 1024 // 大部分进程的权重
# define NICE_OFFSET 20 // 映射时的偏移量
# define SCHED_PERIOD 100 // 调度周期为100ticks，即10ms

extern int sched_prio_to_weight[40];

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state 进程控制块PCB
struct proc {
  uint sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  int pid;                     // Process ID
  struct proc *parent;         // Parent process
  struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
  float vruntime;               // 虚拟运行时间Virtual Runtime将各个优先级进程等效为NICE值为0的进程所运行的时间
  int nice;                    // 进程的NICE值，由sched_prio_to_weight映射到真正的权重，越小越优先
  uint ticks_remain;           // 进程剩余的ticks值，清零后会主动放弃CPU。该值在它被调度之前才会赋值
};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap

#endif // PROC_H
