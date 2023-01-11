#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
int main()
{
    int ret = 0;

    ret = fork();

    if(ret == 0)
    {
        printf("子进程%d\n",getpid());
        return 1;
    }

    else if(ret > 0)
    {
        sleep(3);
        printf("父进程%d\n",getpid());
        waitpid(ret,NULL,0);
    }
    else
        printf("fork error");
    
    return 0;
}