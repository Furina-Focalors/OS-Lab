# OS-Lab
**注意**，该代码是基于开源的xv6操作系统(https://github.com/mit-pdos/xv6-public)修改而成的，在使用时请务必遵守相关协议！

## 实验内容
1. 在QEMU上部署xv6操作系统；
2. 在xv6上实现消息队列；
3. 修改xv6的调度器，在xv6上实现CFS（完全公平调度）。

## 实验目的
1. 熟悉QEMU模拟器的使用，了解操作系统的启动过程，了解xv6的结构
2. 理解消息队列的原理，理解进程间通信的机制
3. 理解CPU进程调度的过程，掌握CFS调度算法的原理

## 实验步骤
### 部署xv6操作系统
首先，在虚拟机VMware Workstation上部署Ubuntu 22，作为实验环境。之后，使用apt-get install qemu-system下载QEMU模拟器。

之后，（根据6.828的教程）使用objdump -i和gcc -m32 -print-libgcc-file-name检查系统编译器工具链的完整性。其中，由于我使用的是64位系统，32位gcc库需要使用sudo apt-get install gcc-multilib手动安装。

然后，从xv6官方的GitHub开源项目下载xv6操作系统的源代码。解压后，在项目目录下打开终端，使用make qemu从模拟器启动xv6，完成xv6操作系统的部署。

### 实现消息队列
消息队列被用作进程间通信，它是在内核空间存放的一个队列，用户进程通过相关的系统调用将消息发送至队列中，并可以由另一个进程接收，从而实现进程间通信。xv6中进程间通信的方法是使用管道（pipe），这种方式虽然简单，但是每次只能发送一个字节，而且不能异步通信。

新建文件msg.h，在其中定义结构体msg作为消息体。其中有四个属性，msg_type代表消息的类型，这里的类型不是基本数据类型，而是发送方自定义的一个整型数，接收方通过这个整数来查找相应的消息；data是一个指向消息内容的指针。为了更高效地操作消息，采用了双向链表结构的消息队列，因此在消息体中包含了pre和next两个指针。
```c
// 消息体
struct msg{
    long msg_type;// 消息的类型
    //uint msg_size;// 消息的长度
    void *data;// 消息的内容
    struct msg *next;// 指向下一个消息
    struct msg *pre;// 指向上一个消息
};
```

定义结构体msg_queue作为消息队列。有三个属性，id是消息队列的唯一标识符；lock是一个自旋锁，它是为了确保对同一个消息队列的访问是同步的；messages用于存放消息，这里为消息队列分配了最大长度。此外，为了统一对空队列和非空队列的操作，在创建队列时将messages\[0\]设置为头节点，它不存放任何消息。
```c
// 消息队列
struct msg_queue{
    uint id;// 消息队列的id
    struct spinlock lock;// 向消息队列的写入必须是同步的
    struct msg *messages[MAX_MSG_COUNT];// 存放消息的空间，index=0处默认存放头节点first
};
```
此外，还需要有一个数组存放消息队列的指针，用于维护系统中已经创建的消息队列。这个数组也需要一个自旋锁，用于同步访问。
```c
struct msg_queue *msq_table[MAX_MSQ_COUNT];
struct spinlock msq_table_lock;
```
用户进程对消息队列的操作有四种：创建消息队列、删除消息队列、发送消息、接收消息，这些方法在msg.c中实现。
#### 创建消息队列
在创建消息队列时，首先要获取消息队列表的锁，以确保操作的原子性。之后，寻找表中一个空闲项，使用xv6提供的函数kalloc()分配一块内存用于存放队列。之后，初始化消息队列的各项属性，并创建头节点。

#### 删除消息队列
删除消息队列时，首先要删除其中的所有消息，并使用kfree()释放相应的内存空间，否则队列删除后将没有指针指向这部分消息所在的内存，从而导致内存泄漏。在删除全部消息之后再删除消息队列。

#### 发送消息
用户发送消息时，需要额外提供消息队列的id和消息的类型。队列id被用于在消息队列表中查找相应的消息队列，消息类型用于接收方接收时作为标识符。在这个函数中，先根据msq_id查找消息队列，再找到消息队列的末端，将消息插入队列中。这个过程需要获取对应消息队列的锁。

#### 接收消息
接收消息的过程和发送消息类似，只是查找到消息后，将其存放的内容返回给用户，并释放内核中这部分内存。

#### 新增系统调用
在实现这四个方法之后，还需要为用户提供相应的接口来调用这些方法，即系统调用。因此，对于这四个功能，需要新增四个系统调用。

首先，在syscall.h下编写系统调用号的宏定义：
```c
#define SYS_createmsq 22
#define SYS_deletemsq 23
#define SYS_msgsnd 24
#define SYS_msgrcv 25
```

之后，在syscall.c中有一个结构static int (*syscalls[])(void)，它构建了系统调用号和系统调用函数之间的映射关系，将新增的四个系统调用号分别关联到相应的函数名。
```c
static int (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
[SYS_exit]    sys_exit,
[SYS_wait]    sys_wait,
[SYS_pipe]    sys_pipe,
[SYS_read]    sys_read,
[SYS_kill]    sys_kill,
[SYS_exec]    sys_exec,
[SYS_fstat]   sys_fstat,
[SYS_chdir]   sys_chdir,
[SYS_dup]     sys_dup,
[SYS_getpid]  sys_getpid,
[SYS_sbrk]    sys_sbrk,
[SYS_sleep]   sys_sleep,
[SYS_uptime]  sys_uptime,
[SYS_open]    sys_open,
[SYS_write]   sys_write,
[SYS_mknod]   sys_mknod,
[SYS_unlink]  sys_unlink,
[SYS_link]    sys_link,
[SYS_mkdir]   sys_mkdir,
[SYS_close]   sys_close,
// The following lines are what we added here.
[SYS_createmsq] sys_createmsq,
[SYS_deletemsq] sys_deletemsq,
[SYS_msgsnd]  sys_msgsnd,
[SYS_msgrcv] sys_msgrcv,
};
```

在相同文件下写上相应系统调用的函数头。声明为extern是因为我们将在其他文件下实现这些函数。
```c
extern int sys_createmsq(void);
extern int sys_deletemsq(void);
extern int sys_msgsnd(void);
extern int sys_msgrcv(void);
```

之后，在user.h中定义提供给用户调用的函数接口。需要注意的是，这些函数的名字需要和相应系统调用号宏定义的后半部分相同。例如，创建消息队列的系统调用号的宏定义名是SYS_createmsq，则用户接口的函数名必须声明为createmsq。
```c
/**
 * 创建消息队列
 * @param uint qid: 消息队列的id
 * @return 创建成功返回0，否则返回-1
 */
int createmsq(uint qid);
/**
 * 发送消息
 * @param uint qid:      消息队列的id
 * @param const_void *msg:  指向用户空间消息的指针
 * @param long mtype:    消息的类型
 * @param int msize: 消息的大小
 * @return 发送成功返回0，否则返回-1
 */
int msgsnd(uint qid, const void *msg, long mtype, int msize);
/**
 * 接收消息
 * @param uint qid:      消息队列的id
 * @param const_void *msg:  指向用户空间消息的指针
 * @param long mtype:    消息的类型
 * @param int msize: 消息的大小
 * @return 接收成功返回0，否则返回-1
 */
int msgrcv(uint qid, void *msg, long mtype, int msize);
/**
 * 删除消息队列
 * @param uint qid: 消息队列的id
 * @return 删除成功返回0，否则返回-1
 */
int deletemsq(uint qid);
/**
 * 更改进程的nice值
 * @param pid: 进程号
 * @param val: 新的nice值
 * @return
 */
```

在usys.S中，我们需要添加新的系统调用入口点，以注册到中断向量表中，这样内核才能正常处理系统调用。
```asm
SYSCALL(createmsq)
SYSCALL(deletemsq)
SYSCALL(msgsnd)
SYSCALL(msgrcv)
```

之前提到，用户接口名必须和系统调用号宏定义名的后半部分相同，这是因为，在usys.S中，movl $SYS_ ## name, %eax; \这一行代码将传入的参数name与"SYS_"拼接成了系统调用号，因此要求名称必须相同，这样才能组合成正确的系统调用号

最后，在sysproc.c文件下实现四个系统调用函数。系统调用函数不接受任何参数，使用syscall.c中实现的argint()和argptr()方法来获取用户传入的参数，这两个函数均有两个参数，第一个代表用户传入的参数位置，0代表第一个参数；第二个参数是用于接收参数值的变量地址。在这里，系统调用函数调用先前实现的几个方法，从而实现相应的功能。

### 实现CFS调度算法
完全公平调度（Completely Fair Schedule, CFS）算法是Linux操作系统使用的一种调度算法，它能够公平地分配CPU使用时间。

在一个调度周期内，CPU按进程的权重占队列上全部进程权重的比例分配实际运行时间，如下所示：
$$ wallTime = schedPeriod * weight/sumOfWeights $$

其中，wall_time是实际运行时间。权重weight是通过一个整数（被称为nice值）映射到一个数组获取的，这个数组定义在proc.c中：
```c
int sched_prio_to_weight[40] = {
        /* -20 */     88761,     71755,     56483,     46273,     36291,
        /* -15 */     29154,     23254,     18705,     14949,     11916,
        /* -10 */      9548,      7620,      6100,      4904,      3906,
        /*  -5 */      3121,      2501,      1991,      1586,      1277,
        /*   0 */      1024,       820,       655,       526,       423,
        /*   5 */       335,       272,       215,       172,       137,
        /*  10 */       110,        87,        70,        56,        45,
        /*  15 */        36,        29,        23,        18,        15,
};
```
nice是一个[-20, 19]范围内的整数，它加上20后即可映射到上面这个数组。可以看出，nice值越小，优先级越高。这些数值是经过公式$ weight=\frac{1024}{1.25^{nice}} $计算出来的，在这种情况下，进程的nice每增加1，它在一个调度周期内占用CPU的时间就可以增加约10%。

CFS的“完全公平”体现在它引入了虚拟运行时间vruntime这个概念，它的计算方法如下所示：

$$ vruntime += (wallTime * nice_0) / weight $$

其中$ nice_0 $是nice值为0时的权重值，等于1024. vruntime在进程创建时被初始化为0，随着进程的实际执行时间增加，vruntime也会增加，但不同权重的进程虚拟运行时间增加的速度不同。事实上，联立两个式子可以得到，$ vruntime += schedPeriod * 1024/sumOfWeights $，这说明，在一个调度周期内，假设没有新的进程进入就绪状态，也没有进程在时间片被消耗完之前结束执行或被阻塞，每个进程的虚拟运行时间是相同的，这就是CFS的公平之处。因此，CFS调度的基本思想是，每次选择虚拟运行时间最短的进程运行。

Linux中，CFS调度队列是使用红黑树实现的，但在本实验中，我使用了最小堆结构实现调度队列，这是因为堆结构简单，且插入和删除操作的时间复杂度同样能达到O(logn)，检索最小值的时间复杂度为O(1)，具有足够优秀的性能。

最小堆的结构体如下所示，定义在minheap.h中，weight_sum是队列中进程的权重和，用于计算分配的时间片长度；lock用于确保堆操作的原子性。
```c
struct MinHeap {
    struct proc *processes[MAX_HEAP_SIZE];
    int size;
    struct spinlock lock;
    unsigned int weight_sum;
};
```

之后，在proc.h中的cpu结构体上加上其对应的调度队列，如下所示。
```c
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
```

在main.c中添加一个函数，用于初始化每个CPU上的消息队列：
```c
// 初始化每个cpu的调度队列
static void init_proc_queue(){
    struct cpu *c;
    for(c = cpus; c < cpus+ncpu; c++){
        initializeHeap(&(c->proc_queue));
    }
}

// Bootstrap processor starts running C code here.
// Allocate a real stack and switch to it, first
// doing some setup required for memory allocator to work.
int
main(void)
{
  kinit1(end, P2V(4*1024*1024)); // phys page allocator
  kvmalloc();      // kernel page table
  mpinit();        // detect other processors
  lapicinit();     // interrupt controller
  seginit();       // segment descriptors
  picinit();       // disable pic
  ioapicinit();    // another interrupt controller
  consoleinit();   // console hardware
  uartinit();      // serial port
  pinit();         // process table
  tvinit();        // trap vectors
  binit();         // buffer cache
  fileinit();      // file table
  ideinit();       // disk 
  startothers();   // start other processors
  kinit2(P2V(4*1024*1024), P2V(PHYSTOP)); // must come after startothers()
  init_proc_queue(); // schedule queue
  userinit();      // first user process
  mpmain();        // finish this processor's setup
}
```

之后，在proc.h中为proc结构体增加了三个属性，如下所示。vruntime用于记录进程的虚拟运行时间，nice用于映射到进程的权重，ticks_remain代表进程剩余可用的ticks值，这个属性代表进程剩余的实际执行时间，将在后面详细叙述。这三个属性值会在allocproc()方法中被初始化为0.
```c
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
```

在进程进入RUNNABLE（就绪）状态时，需要将进程加入当前CPU的调度队列中；而当RUNNABLE状态的进程进入其他状态（RUNNING, SLEEPING）时，需要将其移出队列。因此，修改了以下函数：
1. **userinit()**。这是创建第一个用户进程的方法。当它进入RUNNABLE状态时，需要将其加入就绪队列。
2. **fork()**。xv6操作系统的所有用户进程都是通过userinit()生成的第一个用户进程调用fork()生成的（事实上，是从进程池选择一个UNUSED状态的进程作为相应的进程），因此，当fork()生成的进程进入RUNNABLE状态后，需要将其加入就绪队列，如下所示。其中，子进程继承父进程的nice值。
3. **wakeup1()**。这个函数会将一个SLEEPING状态的进程唤醒，进入RUNNABLE状态，因此需要将进程加入调度队列。

之后，我们需要实现调度器函数scheduler()，这是进程调度的核心。CPU调度的大致过程如下：
1. 调度器scheduler通过调度算法选择一个进程
2. 调度器将其变为RUNNING状态，并通过上下文切换切换到对应进程执行
3. 进程执行期间触发时钟中断，内核调用yield()方法使进程放弃CPU，并回到调度器进程
4. 调度器将当前运行的进程设置为NULL，并开始新一轮调度

逐步实现这一过程。首先，基于之前实现的最小堆，取堆顶的进程，它即是虚拟运行时间最小的进程。然后，根据这个进程的nice值，为其分配相应的时间片，SCHED_PERIOD是调度周期，Linux中的调度周期是根据进程数动态调整的，这里简单起见将其设置为固定100ticks。

之后，将这个进程设置为当前运行的进程，将其状态设置为RUNNING（在调度开始时已经将它取出就绪队列，因此不用再delete一次），并使用swtch()函数进行上下文切换，执行用户进程。
```c
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;

  for(;;){
    // Enable interrupts on this processor.
    sti();

    // CFS schedule
    acquire(&ptable.lock);
    p = deleteMin(&(c->proc_queue));// 取堆顶的进程，它是虚拟运行时间最小的进程
    if(!p) {// 没有进程等待
      release(&ptable.lock);
      continue;
    }
    // 分配时间片，按照进程的权重占总权重的比例
    // wall_time = sched_period * weight/(SUM_OF_WEIGHTS)
    p->ticks_remain = SCHED_PERIOD * sched_prio_to_weight[p->nice+NICE_OFFSET] / c->proc_queue.weight_sum;

    // Switch to chosen process.  It is the process's job
    // to release ptable.lock and then reacquire it
    // before jumping back to us.
    c->proc = p;
    switchuvm(p);
    p->state = RUNNING;

    swtch(&(c->scheduler), p->context);// 此时切换到用户进程p
    switchkvm();

    // Process is done running for now.
    // It should have changed its p->state before coming back.
    c->proc = 0;
    release(&ptable.lock);

  }
}
```

当系统启动后，会不断触发时钟中断。中断处理程序检测到时钟中断后，会将一个全局变量ticks自增，它可以代表启动后经过的时间。在做其他修改前，先修改lapic.c中的lapicinit函数的如下行（图中选中行），这是系统触发时钟中断的间隔，在原先的设置中，每10ms触发一次时钟中断。为了更精确的分配时间片，从而体现nice的作用，这里将时钟中断的触发频率增大了100倍，即1tick=0.1ms。
```c
void
lapicinit(void)
{
  if(!lapic)
    return;

  // Enable local APIC; set spurious interrupt vector.
  lapicw(SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));

  // The timer repeatedly counts down at bus frequency
  // from lapic[TICR] and then issues an interrupt.
  // If xv6 cared more about precise timekeeping,
  // TICR would be calibrated using an external time source.
  lapicw(TDCR, X1);
  lapicw(TIMER, PERIODIC | (T_IRQ0 + IRQ_TIMER));
  lapicw(TICR, 100000);// 0.1ms

  // other operations
}
```
在这里，时钟中断还有另一个作用。在中断处理程序中，如果检测到时钟中断，可以递减当前执行进程（如果有）的ticks_remain值，同时增加其虚拟运行时间，当进程的ticks_remain耗尽，就调用yield()方法，强制其放弃CPU，切换回调度器进程。以下为trap.c中的trap()函数的一部分。
```c
  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(myproc() && myproc()->state == RUNNING &&
     tf->trapno == T_IRQ0+IRQ_TIMER){
      myproc()->ticks_remain--;
      myproc()->vruntime += (float)NICE_0_LOAD / (float)sched_prio_to_weight[myproc()->nice+NICE_OFFSET];// 更新虚拟运行时间
      if(!myproc()->ticks_remain)// 时间片用完时，主动放弃cpu
        yield();
  }
```
yield()方法将当前执行的进程切换为RUNNABLE状态，并重新加入消息队列，之后使用sched()方法切换回调度器进程，如下所示。
```c
// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE; // 表示退出运行状态进入就绪状态，即放弃执行
  insert(&(mycpu()->proc_queue), myproc()); // 将进程重新加入调度队列
  sched();
  release(&ptable.lock);
}
```
最后，调度器清空存放当前执行进程的指针，并准备进行下一次调度。

由于进程默认的nice值均为0，需要为用户提供一个接口（系统调用），供用户修改进程的nice值。使用与前一个实验相同的方法新增一个系统调用updnice，用户传入进程号和nice值。

