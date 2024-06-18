#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


int sys_createmsq(void){
    uint qid;
    if(argint(0, (int*)&qid) == -1) return -1;// 获取用户传入的第一个参数为int格式
    return create_msg_queue(qid);
}

int sys_deletemsq(void){
    uint qid;
    if(argint(0, (int*)&qid) == -1) return -1;// 获取用户传入的第一个参数为int格式
    return delete_msg_queue(qid);
}

int sys_msgsnd(void){
    uint qid = 0;
    void *msg = 0;
    long mtype = 0;
    int msize = 0;
    if(argint(3, &msize) == -1 ||
    argptr(1, (char**)&msg, msize) == -1 ||
    argint(0, (int*)&qid) == -1 ||
    argint(2, (int*)&mtype) == -1) return -1;
    return msg_send(qid, msg, mtype);
}

int sys_msgrcv(void){
    uint qid = 0;
    void **msg = 0;
    long mtype = 0;
    int msize = 0;
    if(argint(3, &msize) == -1 ||
       argptr(1, (char**)&msg, msize) == -1 ||
       argint(0, (int*)&qid) == -1 ||
       argint(2, (int*)&mtype) == -1) return -1;
    msg_receive(qid, msg, mtype);
    if(!*msg)return -1;
    return 0;
}

int sys_updnice(void){
    int pid, nice;
    argint(0, &pid);
    argint(1, &nice);
    return update_nice(pid, nice);
}