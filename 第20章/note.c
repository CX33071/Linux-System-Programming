//注册信号处理函数
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
    void handle_sigint(int sig){
        printf("捕获到信号SIGINT，但不退出\n");
    }
    int main(){
        if(signal(SIGINT,handle_sigint)==SIG_ERR){
            perror("signal failed");
            return 1;
        }
        while(1){
            sleep(1);//按ctrl+c不会终止程序，只打印信息，按ctrl+\发送SIGQUIT终止程序
        }
        return 0;
    }
//保证信号处理函数的可重入性
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
    volatile sig_atomic_t got_signal = 0;
    void handle_sigusr1(int sig){
        got_signal = 1;//仅设置标志位
        char msg[] = "收到SIGUSR1信号";
        write(2, msg, sizeof(msg));
    }
    int main(){
        if(signal(SIGUSR1,handle_sigusr1)==SIG_ERR){
            perror("signal");
            return 1;
        }
        while(!got_signal){
            pause();
        }
        return 0;
    }
//kill的实现
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
int main(int argc,char*argv[]){
    if(argc!=3){
        fprintf(stderr, "用法：%s <pid> <sig>", argv[0]);
        return 1;
    }
    pid_t target_pid = atoi(argv[1]);
    int sig = atoi(argv[2]);
    if(kill(target_pid,sig)==-1){
        perror("kill failed");
        return 1;
    }
    printf("已向进程%d发送信号%d\n", target_pid, sig);
    return 0;
}
//检查进程的存在
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
int main (int argc,char*argv[]){
    if(argc!=2){
        fprintf(stderr, "用法：%s <pid>\n", argv[0]);
        return 1;
    }
    pid_t target_pid = atoi(argv[1]);
    if(kill(target_pid,0)==0){
        printf("进程%d存在\n", argv[1]);
    }else{
        if(errno==ESRCH){
            printf("进程%d不存在\n", argv[1]);
        }else{
            perror("kill");
            return 1;
        }
    }
    return 0;
}
//raise()
#include <signal.h>
#include <stdio.h>
void handle_signal(int sig){
    printf("进程%d收到了自己发送的SIGUSR1\n", getpid());
}
int main(){
    if(signal(SIGUSR1,handle_signal)==SIG_ERR){
        perror("signal failed");
        return 1;
    }
    if(raise(getgid())!=0){
        perror("raise");
        return 1;
    }
    return 0;
}
//显示信号描述
#include <string.h>
#include <signal.h>
#include <stdio.h>
int main(){
    int sig = SIGINT;
    printf("信号%d的描述%s\n", sig, strsignal(sig));
    psignal(sig, "收到信号");
    return 0;
}
//信号集
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
int main(){
    // sigset_t sigset;
    // sigemptyset(&sigset);
    // sigaddset(&sigdet, SIGINT);
    // return 0;
}
//设置信号掩码
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
int main(){
    // sigset_t new_mask, old_mask;
    // sigemptyset(&new_mask);
    // sigaddset(&new_mask, SIGINT);
    // if(sigprocmask(SIG_BLOCK,&new_mask,&old_mask)==-1){//设置新的信号掩码，并保存旧掩码
    //     perror("sigprocmask");
    //     return 1;
    // }
    // if(sigprocmask(SIG_SETMASK,&old_mask,NULL)==-1){//恢复旧的信号掩码
    //     perror("sigprocmask");
    //     return 1;
    // }
    return 0;
}
//获取pending信号
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
int main(){
    // sigset_t new_mask, pending_mask;
    // sigemptyset(&new_mask);
    // sigaddset(&new_mask, SIGINT);
    // sigprocmask(SIG_BLOCK, &new_mask, NULL);
    // if(sigpending(&pending_mask)==-1){
    //     perror("sigpending");
    //     return 1;
    // }
    // if(sigismember(&pending_mask,SIGINT)){
    //     printf("SIGINT处于pending状态");
    // }
    // sigprocmask(SIG_UNBLOCK, &new_mask, NULL);
    // return 0;
}
//验证标准信号不排队
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
volatile sig_atomic_t count = 0;
void handle_sigusr1(int sig){
    count++;
}
int main(){
    // if(signal(SIGUSR1,handle_sigusr1)==SIG_ERR){
    //     perror("signal");
    //     return 1;
    // }
    // sigset_t mask;
    // sigemptyset(&mask);
    // sigaddset(&mask, SIGUSR1);
    // sigprocmask(SIG_BLOCK, &mask, NULL);
    // printf("在另一个进程执行两次kill SIGUSR1");
    // sleep(3);
    // sigprocmask(SIG_UNBLOCK, &mask, NULL);
    // printf("%d", count);
    // return 0;
}
//sigaction()注册信号处理函数
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
void handle_sigint(int sig){
    printf("捕获到SIGINT");
}
int main(){
    // struct sigaction sa;
    // sigemptyset(&sa.sa_mask);
    // sa.sa_handle = handle_sigint;
    // sa.sa_flags = 0;
    // if(sigaction(SIGINT,&sa,NULL)==-1){
    //     perror("sigaction");
    //     return 1;
    // }
    // while(1){
    //     pause();
    // }
    // return 0;
}