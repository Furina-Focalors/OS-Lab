//
// Created by Furina on 2023/12/2.
//
#include "types.h"
#include "stat.h"
#include"user.h"

int
main(int argc, char *argv[]){
    int p1[2], p2[2];// 两个管道，用于进程间通信，p[0]表示标准输入流，p[1]表示标准输出流，此处未用p[2]，表示标准错误流
    char buffer[] = {'l'};
    int len = sizeof(buffer);
    pipe(p1);// 为p1的输入输出流创建管道，用于进程间通信
    pipe(p2);
    if (fork() == 0) {
        // 关闭p1的输出流和p2的输入流
        close(p1[1]);
        close(p2[0]);
        if (read(p1[0], buffer, len) != len) {// 从p1读
            printf(2, "child read error!\n");
            exit();
        }
        printf(2, "%d: received ping\n", getpid());
        if (write(p2[1], buffer, len) != len) {// 写p2
            printf(2, "child write error\n");
            exit();
        }
        exit();
    } else {
        // 关闭p1的输入流和p2的输出流
        close(p1[0]);
        close(p2[1]);
        if (write(p1[1], buffer, len) != len) {// 写p1
            printf(2, "parent write error!\n");
            exit();
        }
        if (read(p2[0], buffer, len) != len) {// 从p2读
            printf(2, "parent read error!\n");
            exit();
        }
        printf(2, "%d: received pong\n");
        wait();
        exit();
    }
}