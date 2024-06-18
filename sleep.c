//
// Created by Furina on 2023/12/1.
//
#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf(2, "Usage: sleep time...\n");
        exit();
    }
    int time = atoi(argv[1]);
    sleep(time);
    printf(2, "Sleep finished.\n");// declared in user.h
    exit();
}