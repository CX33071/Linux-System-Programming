//进程退出原因
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
int main(){
    pid_t pid =fork();
    if(pid==-1){
        perror("fork");
        return 1;
    }else if(pid==0){
        while(1){
            sleep(1);
        }
    }else{
        kill(pid, SIGHUP);
        int status;
        waitpid(pid, &status, 0);
        if(WIFEXITED(status)){
            printf("exit code:%d\n", WEXITSTATUS(status));
        }else{
            int sig = WTERMSIG(status);
            printf("%d", sig);
        }
    }
    return 0;
}
//注册退出处理程序
#include <stdio.h>
#include <stdlib.h>
void handle1(){
    printf("1\n");
}
void handle2(){
    printf("2\n");
}
int main(){
    atexit(handle1);
    atexit(handle2);
    return 0;//等价与exit(0)
}