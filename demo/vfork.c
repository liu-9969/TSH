#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

void* func(void* query)
{
    int ret = 0;
    printf("handle pthread func run pid:%d\n",getpid());

    ret = vfork();

    if(ret < 0)
    {
        printf("vfork error\n");
    }

    //子进程继续vfork,旨在正确调用setsid()、ioctl()
    if(ret == 0)
    {
        printf("子进程运行\n");
#if 1
        int vret = 0;
        vret = vfork();

        if(vret < 0)
            printf("vfork2 error\n");

        //1.保证外边的父进程可以继续执行// 2.保证新创建的子进程不是进程组组长 // 保证最新进程无控制终端
        if(vret > 0)
        {
            exit(1);
        }

        if(vret == 0)
        {
            // 创建会话
            if (setsid() < 0)
                printf("setsid error\n");
            //调用ioctl 给他设置终端
            else
                printf("会话创建成功\n");
            
            execl("/bin/sh","ls", (char*) 0);
            exit(2);
        }
#else
        execl("/bin/sh","s","-c","ls", (char*) 0);
#endif
    }

    if(ret > 0)
    {
        printf(" handle socket with client\n");
    }

    return NULL;
}


int main()
{

    pthread_t tid;
    pthread_create(&tid, NULL, func, NULL);
    pthread_detach(tid);
    // int ret = 0;
    // ret = fork();
    // if (ret == 0)
    //     execl("/bin/sh","s", "-c", "ls", (char*)0);
    // if (ret > 0)
        // printf("father pro con\n");

   
    printf("模拟accept\n");
    sleep(1);
    
    return 0;
}