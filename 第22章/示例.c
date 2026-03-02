//处理sleep被信号中断和read
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
void sigint_handler(int sig){
    printf("被信号SIGINT中断");
}
int main(){
    signal(SIGINT, sigint_handler);
    int ret = sleep(10);
    if(ret>0){
        printf("被SIGINT打断，还剩%d秒", ret);
    }else if(ret==0){
        printf("sleep正常完成");
    }
    //处理EINTR的通用写法
    // ssize_t n;
    // do{
    //     n=read(fd,buf,sizeof(buf));
    // } while (n == -1 && errno = EINTR);
    return 0;
}
//捕获除以0错误，跳转
#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/types.h>
// sigjmp_buf jmp_buf;
// void sigfpe_handler(int sig){
//     siglongjmp(jmp_buf, 1);
// }
// int main(){
//     signal(SIGFPE, sigfpe_handler);
//     if(sigsetjmp(jmp_buf,1)==0){
//         int a = 10;
//         int b = 0;
//         int c = a / b;//触发SIGFPE信号，printf不会执行
//         printf("%d", c);
//     }else{
//         printf("错误恢复，程序正常运行");
//     }
//     return 0;
// }
//验证普通信号丢失
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
volatile sig_atomic_t count = 0;
void sigusr1_handler(int sig){
    count++;
    printf("收到SIGUSR1信号%d次", count);
    sleep(2);
}
int main(){
    signal(SIGUSR1, sigusr1_handler);
    while(1){
        sleep(1);
    }
}
//发送带数据的实时信号
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
int main(int argc,char*argv[]){
    if(argc!=3){
        fprintf(stderr, "用法:%s <PID> <数据>", argv[0]);
        exit(1);
    }
    pid_t pid = atoi(argv[1]);
    int data = atoi(argv[2]);
    // union sigval value;
    // value.siaval_int = data;
    // if(sigqueue(pid,SIGRTMIN+1,value)==-1){
    //     perror("sigqueue");
    //     exit(1);
    // }
    return 0;
}
//接收实时信号
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
// void sigusr1_handler(int sig){
//     printf("收到实时信号%d,发送进程：%d,附带数据:%d", sig, info->si_pid,
//            info->si_value.sival_int);
// }
// int main(){
//     struct sigaction sa;
//     sa.sa_handler = sigusr1_handler;
//     sigemptyset(&sa.sa_mask);
//     sa.sa_flags = 0;
//     sigaction(SIGUSR1, &sa, NULL);
//     while(1){
//         pause();
//     }
//     return 0;
// }
//使用掩码安全等待信号sigsuspend()
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
volatile sig_atomic_t flag = 0;
void sigusr1_handler(int sig){
    flag = 1;
}
int main(){
    // struct sigaction sa;
    // sa.sa_handler = sigusr1_handler;
    // sigemptyset(&sa.sa_mask);
    // sa.sa_flags = 0;
    // sigaction(SIGUSR1, &sa, NULL);
    // sigset_t new_mask, old_mask;
    // sigfillset(&new_mask);
    // sigdelset(&new_mask.SIGUSR1);
    // sigsuspend(&new_mask);
    // return 0;sigdelset只是修改信号掩码变量，不会改变进程的实际信号掩码，进程的实际信号掩码在sigsuspend执行时才被替换，这期间到来的SIGUSR1会被内核暂存，sigsuspend执行后立马处理
}
//sigtimedwait()超时等待信号
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
int main(){
    // sigset_t set;
    // sigemptyset(&set);
    // sigaddset(&set, SIGUSR1);
    // sigprocmask(SIG_BLOCK, &set, NULL);
    // struct timespec timeout;
    // timeout.tv_sec = 5;
    // timeout.tv_nsec = 0;
    // siginfo_t info;
    // int sig = sigtimedwait(&set, &info, &timeout);
    // if(sig==-1){
    //     perror("sigtimedwait");
    // }else{
    //     printf("收到信号%d", sig);
    // }
    // return 0;
}
//signalfd读取信号
#include <stdio.h>
#include <sys/signal.h>
#include <signal.h>
#include <unistd.h>
int main(){
    // sigset_t set;
    // sigemptyset(&set);
    // sigaddset(&set, SIGINT);
    // sigaddset(&set, SIGUSR1);
    // sigprocmask(SIG_BLOCK, &set, NULL);
    // int sfd = signalfd(-1, &set, 0);
    // if(sfd==-1){
    //     perror("signalfd");
    //     return 1;
    // }
    // struct signalfd_siginfo fdsi;
    // ssize_t n = read(sfd, &fdsi, sizeof(fdsi));
    // if(n!=sizeof(fdsi)){
    //     perror("read");
    //     return 1;
    // }
    // printf("收到信号%d", fdsi.ssi_signo);
    // close(sfd);
    // return 0;
}
//父子进程信号通信
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
volatile sig_atomic_t child = 0;
void sigusr1_handler(int sig){
    child = 1;
}
void sigusr2_handler(int sig){
    sleep(2);
}
int main(){
    pid_t pid = fork();
    if(pid==-1){
        perror("fork");
        return 1;
    }
    if(pid==0){
        signal(SIGUSR2, sigusr2_handler);
        kill(getppid(), SIGUSR1);
        while(1){
            sleep(1);
        }
    }else{
        signal(SIGUSR1, sigusr1_handler);
        while(!child){
            pause();
        }
        kill(pid, SIGUSR2);
        waitpid(pid, NULL, 0);
    }
    return 0;
}