1.间隔定时器：
    内核为每个进程维护的定时器，可周期性或一次触发
    三种类型：ITIMER_REAL   基于真实时间，到期时发送SIGALRM
            ITIMER_VIRTUAL  基于进程在用户态的CPU时间，到期时发送SIGVTALRM
            ITIMER_PROF 基于进程在用户态+内核态的CPU时间，到期发送SIGPROF
    核心函数：setitimer()设置定时器，getitimer()获取当前状态
    #include <sys/time.h>
    int setitimer(int which, const struct itimerval *new_value, struct itimerval *old_value);
    int getitimer(int which, struct itimerval *curr_value);
    结构体itimerval:
    struct itimerval {
    struct timeval it_interval; // 后续触发间隔
    struct timeval it_value;    // 首次触发时间
    };
    用途：一次性定时器(3秒后触发信号，执行一次后任务退出)、周期性定时器(1秒后首次触发，之后每2秒触发一次)，触发5次后关闭定时器
2.定时器的调度及精度
定时器精度受系统时钟频率限制，linux默认hz为100/250
定时器可能会延迟：内核调度延迟：定时器到期时进程可能未被调度执行
               系统繁忙时定时器会触发延迟
对时间精度要求高的场景，应使用clock_nanosleep()或timerfd API而非setitimer()
3.为阻塞操作设置超时限制
使用alarm()或setitimer()为阻塞操作设置超时，避免永久阻塞
在阻塞操作前设置定时器，若操作超时，定时器发送信号中断阻塞操作
alarm()触发固定信号SIGALRM，专门用于设置秒级一次性定时器，指定n秒后发送信号，返回值为上一个定时器剩余的秒数，无则返回0，alarm(0)取消定时器，
4.sleep时进程休眠指定秒数
sleep   可被信号中断、被信号中断返回剩余时间、秒级精度
nanosleep   可被信号中断、被信号打断返回-1,并设置errno，将剩余时间储存在rem参数当中
5.POSIX时钟
- 获取时钟的值clock_gettime
支持多种时种类型
#include <time.h>
int clock_gettime(clockid_t clk_id,struct timespec*tp)
CLOCK_REALTIME系统实时时间，可被用户修改
CLOCK_MONOTONIC单调递增时间，从系统启动开始，不可修改
- 设置时钟的值clock_settime
需要特权CAP_SYS_TIME
- 获取特定进程或线程的时钟ID
CLOCK_PROCESS_CPUTIME_ID进程的CPU时间
CLOCK_THREAD_CPUTIME_ID线程的CPU时间
- clock_nanosleep高分辨率休眠的改进版
支持绝对时间和相对时间，flags:TIMER_ABSTIME表示绝对时间，否则为相对时间
6.POSIX间隔式定时器
- 创建定时器timer_create()
创建基于POSIX时钟的定时器，支持信号等多种通知方式
参数：时种类型，指定通知方式，时间
timer_settime配置定时器
- 配置和解除定时器timer_settime()
启动或修改定时器的触发时间和间隔
- 获取定时器当前剩余时间和间隔timer_gettime()
- 删除定时器timer_delete()
int timer_delete(timer_t timerid)
- 定时器到期时，可发送指定信号并可携带额外数据
- 若定时器到期时，进程未处理前一个信号，会导致溢出，可通过timer_getoverrun()获取溢出次数
- 通过线程通知
7.利用文件描述进行通知的定时器timerfd API
timerfd:将定时器映射为文件描述符，通过read()读取定时器到期事件，适合在epoll等事件循环中使用
timerfd_create创建定时器文件描述符
timerfd_settime设置定时器
timerfd_gettime获取定时器状态
