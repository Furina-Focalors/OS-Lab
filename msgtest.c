//
// Created by Furina on 2023/12/5.
//

#include "types.h"
#include "user.h"

int main(int argc, char* argv[]){
    if(fork()){// parent
        int res = createmsq(123);
        if(res == -1){
            printf(2,"msq_create failed.\n");
            exit();
        }
        printf(2,"msq_create succeeded.\n");
        int my_msg = 182375;
        int res2 = msgsnd(123,&my_msg,182375, sizeof(int*));
        if(res2 == -1){
            printf(2,"msg_send failed.\n");
            exit();
        }
        printf(2, "msg_send succeeded.\n");
        wait();
    }
    else{// child
        sleep(10);// wait for msg to be sent
        int *msg = 0;
        int res3 = msgrcv(123, &msg, 182375, sizeof(int*));
        if(res3 == -1){
            printf(2,"msg_receive failed.\n");
            exit();
        } else if(!msg){
            printf(2,"msg_receive succeeded, but the pointer is still null.\n");
            exit();
        } else{
            printf(2, "msg_receive succeeded, the msg is %d\n", *msg);

            int res4 = deletemsq(123);
            if(res4 == -1){
                printf(2, "msq_delete failed.\n");
                exit();
            } else{
                printf(2, "msq_delete succeeded.\n");
                exit();
            }
        }
    }
}