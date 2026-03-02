//周期性定时器
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
volatile sig_atomic_t count = 0;
void timer_handler(int sig) {
    printf("定时器触发%d次", ++count);
}
int main(){
    signal(SIGALRM, timer_handler);
    struct itimerval timer;
    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 500000;
    setitimer(ITIMER_REAL, &timer, NULL);
    while(1){
        pause();
    }
    return 0;
}
//read超时设置
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
void alarm_handler(int sig){

}
int main(){
    signal(SIGALRM, alarm_handler);
    char buf[100];
    alarm(5);
    ssize_t n = read(1, buf, sizeof(buf));
    if(n==-1){
        if(errno=EINTR){
            printf("读取超时");
        }else{
            perror("read");
        }
    }else{
        printf("读取到%s", buf);
    }
    alarm(0);
    return 0;
}
//nanosleep休眠
#include <stdio.h>
#include <time.h>
#include <errno.h>
int main(){
    struct timespec req, rem;
    req.tv_sec = 2;
    req.tv_nsec = 500000000;
    // while(nanosleep(&req,&rem)==-1&&errno=EINTR){
    //     req = rem;
    // }
    return 0;
}
//获取POSIX时钟的值
#include <stdio.h>
#include <time.h>
int main(){
    struct timespec ts;
    // clock_gettime(CLOCK_MONOTONIC, &ts);
    printf("单调时间:%ld秒%ld纳秒", ts.tv_sec, ts.tv_nsec);
    return 0;
}
//通过POSIX定时器接口创建一个周期性定时器
//用timer_create创建基于CLOCK_MONOTONIC单调时钟的定时器，指定触发时发送SIGUSR1信号
//用timer_settime配置定时器，1秒后首次触发，之后每隔1秒触发一次
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

void timer_handler(int sig) {
    printf("POSIX定时器触发\n");
}

int main() {
    // struct sigaction sa;
    // sa.sa_handler = timer_handler;
    // sigemptyset(&sa.sa_mask);
    // sa.sa_flags = 0;
    // sigaction(SIGUSR1, &sa, NULL);

    // timer_t timerid;
    // struct sigevent sev;
    // sev.sigev_notify = SIGEV_SIGNAL;
    // sev.sigev_signo = SIGUSR1;
    // timer_create(CLOCK_MONOTONIC, &sev, &timerid);

    // struct itimerspec its;
    // its.it_value.tv_sec = 1;
    // its.it_value.tv_nsec = 0;
    // its.it_interval.tv_sec = 1;
    // its.it_interval.tv_nsec = 0;
    // timer_settime(timerid, 0, &its, NULL);

    // while (1) {
    //     pause();
    // }
    return 0;
}
//timerfd+epoll
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>

int main() {
    // int tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    // struct itimerspec its;
    // its.it_value.tv_sec = 1;
    // its.it_value.tv_nsec = 0;
    // its.it_interval.tv_sec = 1;
    // its.it_interval.tv_nsec = 0;
    // timerfd_settime(tfd, 0, &its, NULL);

    // int epfd = epoll_create1(0);
    // struct epoll_event ev;
    // ev.events = EPOLLIN;
    // ev.data.fd = tfd;
    // epoll_ctl(epfd, EPOLL_CTL_ADD, tfd, &ev);

    // while (1) {
    //     struct epoll_event events[1];
    //     int n = epoll_wait(epfd, events, 1, -1);
    //     if (n > 0 && events[0].data.fd == tfd) {
    //         uint64_t expirations;
    //         read(tfd, &expirations, sizeof(expirations));
    //         printf("定时器到期 %llu 次\n", (unsigned long long)expirations);
    //     }
    // }

    // close(tfd);
    // close(epfd);
    // return 0;
}
