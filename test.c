//
// Created by Furina on 2023/12/28.
//
#include "types.h"
#include "user.h"

int main() {
    int i=0;
    int pid = fork();// xv6的fork()返回子进程的pid
    while(i<2147483647){
        //printf(2," pid %d call for i=%d\n", pid, i);
        i=i+1;
        if(!(i%100000000)){
            printf(2, "pid %d has reached i = %d\n", pid, i);
        }
        if(pid && i==1000000000)updnice(pid, -5);// 子进程提高优先级
        //sleep(1000);
    }

    if(pid)wait();
    exit();
}
