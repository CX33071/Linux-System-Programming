1.核心转储文件
当进程因为致命信号（如SIGSEGV段错误、SIGABRT异常终止）崩溃时，内核会将进程的内存镜像、寄存器状态等信息写入一个文件(默认名为core或core.<pid>),这个文件就是核心转储文件
- 用途：调试，用gdb加载核心转储文件，可定位崩溃代码行
- 通过ulimit -c设置核心转储文件大小，ulimit -c unlimited表示无限制，默认是0,关闭核心转储
- 并非所有信号都会触发核心转储，SIGINT不会，SIGSEGV/SIGABRT/SIGFPE会，可通过man 7 signal查看信号是否带CORE标记
- 可通过prctl()设置核心转储的文件名格式
核心转储文件通常包含敏感数据，生产环境通常关闭，调试环境开启
ulimit -c unlimited
gcc core_demo.c -o core_demo -g
./core_demo
gdb ./core_demo core
bt  输入bt查看崩溃位置
2.信号处理
- 信号处置优先级：
    设置了自定义处理函数->执行处理函数
    设置为忽略SIG_IGN->信号直接丢弃
    设置为默认SIG_DFL->执行信号默认行为
- SIGKILL(9)和SIGSTOP（19）无法被忽略或阻塞或捕获，内核强制执行默认行为，注册处理函数会直接报错
- 默认情况下，内核会在该信号处理汉函数执行期间自动阻塞当前信号，避免同一信号重复触发导致处理函数重入，“阻塞当前信号” 是内核的通用保护机制（防止处理函数重入），标准和非标准信号都遵循这个规则 —— 唯一区别是，非标准信号被阻塞时会排队，而标准信号会被丢弃
3.可中断和不可中断进程睡眠状态
可中断：标志S，能被信号唤醒，如sleep()/wait()/read()
    进程收到信号立即唤醒并中断当前系统调用，返回-1,errno=EINTR
不可中断：标志D，不能被信号唤醒，如内核操作(磁盘IO、同步内存)
    进程在执行关键内核操作，此时不会相应信号，直到操作完成，避免数据损坏
kill无法终止D状态进程，编写IO等程序时要出时EINTR错误，重新调用read()/sleep()
4.硬件产生的信号
硬件异常触发的信号是由进程自身操作引发的而非外部发送叫做同步信号
段错误(访问非法内存)    SIGSEGV（11）
除以0                 SIGFRE(8)
执行无效机器码          SIGILL(4)
内存对齐错误            SIGBUS(7)
默认行为都是终止+核心转储
信号处理函数执行时，进程上下文就是触发异常的指令位置
这类信号的默认行为是终止进程，即使捕获处理函数，若未修复异常进程继续执行仍会触发信号，所以硬件信号通常是程序bug的表现，捕获这类信号的目的就是优雅退出，而非恢复执行
嵌入式/内核开发中可通过siglongjmp()从信号处理函数跳转到安全位置，但用户态程序不推荐，易造成内存泄漏
5.信号分为同步和异步，区别是是否由进程自身行为引发
同步信号    进程自身执行的指令触发  SIGSEGV、SIGFPE、SIGILL
异步信号    外部事件如其他进程/内核触发 SIGINT、SIGCHLD
同步信号的处理：信号处理函数执行时进程停在触发异常的指令处，若直接返回会重新执行该指令导致信号再次触发
异步信号的处理：信号处理函数执行完后进程回到被中断的指令继续后续代码
同步信号本质是进程自身的错误，异步信号的本质是外部的通知
处理同步信号时如要避免无限循环需用siglongjmp()跳转到安全位置或直接退出程序
6.信号传递时机：内核不会立即传递信号，进程从内核态切换到用户态的时候检查待处理信号，比如sleep，内核会在sleep返回前检查并处理信号，sleep属于可中断睡眠，内核可随时唤醒，被信号唤醒后进程会立即进入准备返回用户态前的检查点，处理信号并提前返回，不会等定时器到期
    信号传递顺序：相同优先级信号先到先处理
                不同优先级信号，实时信号34-64优先级高于普通信号1-31,实时信号按编号从高到低传递
    若进程此刻有多个待处理信号，SIGKILL/SIGSTOP会优先处理
    普通信号不排队会丢失，实时信号排队不会丢失
7.signal()可移植性差，信号处理函数执行完后信号处置会重置为默认
推荐使用sigaction()，sigemptyset(&sa.sa_mask)处理期间部阻塞其他信号，sa.sa_mask=0无特殊标志
8.实时信号
- 发送实时信号
    编号36-64,解决了普通信号不排队容易丢失的问题
    sigqueue()发送实时信号，并附带一个整数/指针数据
    int sigqueue(pid_t pid,int sig,const union sigval value);
    实时信号的编号在不同系统中可能不同，推荐用SIGRTMIN+n,避免硬编码,linux下SIGRTMIN=32
- 处理实时信号
    处理实时信号要用sigaction()，并设置SA_SIGINFO标志才能接收附带的数据
    void handler(int sig,siginfo_t *info,void*context);
    info:包含信号的详细信息，如发送进程PID，附带的数据
    context:进程上下文
    实时信号按标号从高到低、同编号按发送顺序传递
9.程序员想在sleep的时候接收信号，屏蔽->检查->解除屏蔽->sleep等待信号来打断
但是这个操作不是原子的，信号可能不在sleep的时候来，可能在sleep前，屏蔽就是为了防止信号在检查后sleep前到来，若信号在解除屏蔽和sleep之间到来，此刻信号已经处理完，但是进入了if要继续sleep导致白等
解决办法：把解决屏蔽+sleep替换成为sigsuspend()的原子操作，彻底消除竞态，信号要么被屏蔽要么被等待，不会错过，有竟态版按ctrl_c后要等sleep结束才退出，修复版按ctrl+c立即退出
10,以同步方式等待信号
同步等待信号指主动调用函数等待信号到达，而非依赖异步的信号处理函数，使用同步函数前需先阻塞目标信号，避免信号被异步处理函数捕获，同步等待信号的又是：避免异步处理函数的重入问题
sigwaitinfo():
            int sigwaitinfo(const sigset_t *set,siginfo_t *info);
    等待指定信号集中的任意信号，返回信号的详细信息，PID
sigtimedwait():
            int sigtimedwait(const sigset_t *set,siginfo_t *info,const struct timespec*timeout);
    增加了超时时间，避免永久阻塞
11.通过文件描述符来获取信号
linux独有的signalfd()可将信号集映射为文件描述符，通过read()读取信号事件，将信号异步通知转换为文件IO同步读取适合在网络编程的epoll/select循环中统一处理信号和IO事件
#include <sys/signalfd.h>
int signalfd(int fd,const sigset_t*mask,int flags);
步骤：阻塞目标信号(避免异步处理)、调用signalfd()创建信号文件描述符、用read()读取信号事件
12.利用信号进行进程间通信
父进程用SIGCHLD监听子进程退出
进程间用SIGUSR1/SIGUSR2传递自定义通知
实时信号传递少量控制数据
信号不适合传递业务数据，复杂IPC推荐用管道、消息队列、共享内存
父进程fork创建子进程后，等待子进程发送SIGUSR1，子进程初始化完发送SIGUSR1给父进程，父进程收到给子进程法SIGUSR2，子进程执行任务完退出，父进程等待子进程退出后结束
13.早期的信号API
System V 信号API：sigset() sigpause()
BSD信号API：sigvec() sigblock()
14.信号等待的安全方式，sigsuspend()原子操作，sigwaitinfo()/sigtmedwait()同步等待，signalfd()文件IO方式

