//wait()阻塞等待
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
int main(){
    pid_t pid = forl();
    if(pid==-1){
        perror("fork");
        return 1;
    }else if(pid==0){
        sleep(2);
        exit(0);
    }else{
        int status;
        pid_t wpid = wait(&status);
        printf("父进程回收了子进程%d\n", wpid);
        if(WIFEXITED(status)){
            printf("子进程正常退出%d\n", WEXITSTATUS(status));
        }
    }
    return 0;
}
//非阻塞等待
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
int main(){
    pid_t pid = fork();
    if(pid==-1){
        perror("fork");
        return 1;
    }else if(fork==0){
        sleep(3);
        exit(0);
    } else {
        int status;
        pid_t wpid = waitpid(pid, &status, WNOHANG);
        if(wpid==0){
            while(!waitpid(pid,&status,WNOHANG)){
                sleep(1);
            }
        }
        if(WIFEXITED(status)){
            printf("%d", WEXITSTATUS(status));
        }
    }
    return 0;
}
//解析状态值
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
int main(){
    pid_t pid = fork();
    if(pid==-1){
        perror("fork");
        return 1;
    }
    if(pid==0){
        while(1){
            sleep(1);
        }
    }else {
        int status;
        pid_t wpid = waitpid(pid, &status, WUNTRACED);
        kill(pid, SIGSTOP);
        if(WIFEXITED(status)){
            printf("%d\n",WEXITSTATUS(status));
        }
        waitpid(pid, &status, 0);
        kill(pid, SIGKILL);
        if(WIFSIGNALED(status)){
            printf("%d\n", WIERMSIG(status));
        }
    }
    return 0;
}
//从信号处理程序中终止进程，信号驱动回收
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
volatile sig_atomic_t child = 0;
void sigkill_handle(int sig){
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0) {//WNOHANG！，如果是0或其他，无WNOHANG，阻塞！
        child = 1;
    }
}
int main(){
    // struct sigaction sa;
    // sigemptyset(sa.sa_mask);
    // sa.sa_handler = sigkill_handle;
    // sa.sa_flags = 0;
    // sigaction(SIGCHLD, &sa, NULL);
    pid_t pid = fork();
    if(pid==-1){
        perror("fork");
        return 1;
    }
    if(pid==0){
        sleep(3);
        exit(0);
    } else {
        while (!child) {
            pause();
        }
    }
    return 0;
}
//SIGCHLD信号的核心用法，兼顾”回收终止子进程“，”避免僵尸“，”不阻塞主循环“
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
volatile sig_atomic_t child = 0;
void sigchld_handler(int sig){
    int status;
    int ret;
    while ((ret=waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        if (WIFEXITED(status)) {
            printf("子进程%d:正常终止，退出码=%d\n", ret, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("子进程%d：被信号%d终止\n", ret, WTERMSIG(status));
        } else if (WIFSTOPPED(status)) {
            printf("子进程%d：被信号%d暂停\n", ret, WSTOPSIG(status));
        }
        child = 1;
    }
    if(ret==-1&&errno!=ECHILD){//处理waitpid错误，排除没有子进程的情况
        perror("waitpid");
    }
}
int main(){
    // struct sigaction sa;
    // sigemptyset(sa.s_mask);
    // sa.sa_handler = sigchld_handler;
    // sa.sa_flags = 0;//捕获暂停/继续
    // sigaction(SIGCHLD, &sa, NULL);
    pid_t pid = fork();
    if(pid==-1){
        perror("fork");
        return 1;
    }
    if(pid==0){
        sleep(3);
        exit(0);
    }else{
        while(!child){
            pause();
        }
    }
    return 0;
}