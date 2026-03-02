//fork()
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
int main(){
    pid_t pid = fork();
    if(pid==-1){
        perror("fork failed");
        return 1;
    }else if(pid==0){
        sleep(1);
        printf("i am child,PID:%d,parent:%d\n", getpid(), getppid());
    }else{
        printf("i am parent,PID:%d,child:%d", getpid(), pid);
    }
    printf("common code:%d\n", getpid());
    return 0;
}
//信号同步避免父子进程竞争
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
volatile sig_atomic_t child = 0;
void handle_sigusr1(int sig){
    child = 1;
}
int main(){
    // struct sigaction sa;
    // sigemptyset(&sa.sa_mask);
    // sa.sa_flags = 0;
    // sa.sa_handle = handle_sigusr1;
    // sigaction(SIGUSR1, &sa, NULL);
    // pid_t pid = fork();
    // if(pid==-1){
    //     perror("fork");
    //     return 1;
    // }else if(pid==0){
    //     printf("child\n");
    //     sleep(3);
    //     kill(getppid(), SIGUSR1);
    //     _exit(1);
    // }else{
    //     printf("parent");
    //     while(!child){
    //         pause();
    //     }
    //     wait(NULL);
    // }
    // return 0;
}