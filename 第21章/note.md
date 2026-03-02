1.设计信号处理器函数
    标准信号不支持排队，一个信号在被处理时又产生多个同类型信号，这些信号会被合并处理函数只被调用一次，这就意味着信号处理函数不能依赖调用次数来统计事件发生的次数，信号处理函数应该只做最小化的工作，避免在信号处理函数中制作任务可能阻塞或耗时的操作，比如设置标志位。
    幂等操作：因为信号可能被合并，处理函数不能依赖调用次数，一个操作 / 函数 / 接口，无论执行 1 次、10 次还是 100 次，最终的结果和执行 1 次的结果完全一样，且不会产生额外的副作用
    可重入函数：在执行过程中被中断，中断返回后继续执行并不会产生错误的函数
    异步信号安全函数：在信号处理函数中可以安全调用的函数
    常见非安全函数：printf(),malloc(),free(),他们可能使用数据结构或锁，在被信号中断时会导致数据不一致
    sig_atomic_t是一个整数类型，读写操作是原子的，不会被信号中断
2.终止信号处理器函数的其他方法
- 在信号处理器函数中执行非本地跳转
    setjmp()/longjmp()可以在信号处理函数中实现本地跳转，直接调回主程序的某个执行点，但是如果信号处理函数中断了一个非可重入函数malloc，跳转后导致内部状态不一样，所以推荐使用sigsetjmp()/siglongjmp()，它们会保存和恢复信号掩码
    static sigjmp_buf是一个结构体，用于存储程序执行的上下文，供后续siglongjmp()跳转使用，声明为static保证生命周期贯穿镇个程序，信号处理函数和主函数中都能访问
    siglongjmp(env,1)作用：跳回sigsetjmp(env,1)执行的位置，并让sigsetjmp()的返回值变为1(第2个参数)，和普通longjmp()的区别：异步信号安全函数
    sigsetjmp(env,1)第1个参数用于保存当前程序的执行上下文，第2个参数1表示保存信号掩码（保证跳转后信号处理逻辑不丢失），0表示不保存，被siglongjmp()跳转后返回siglongjmp()的第2个参数,第一次调用的作用：保存上下文
    普通信号退出：标记flags=1(退出),但是要把这层循环跑完再次回到while判断条件时才退出，而这两个函数直接跳到循环外
- 异常终止进程abort()
    abort()函数向进程发送SIGABRT信号，默认信号是终止进程并生成核心转储文件，即使SIGABRT被捕获，abort()也会在处理函数返回后终止进程，哪怕注册了处理函数忽略SIGABRT信号也退出，不会刷新缓冲区导致部分数据丢失，程序直接退出，和exit一样，但是exit会刷新缓冲区
3.在备选栈中处理信号sigaltstack()
    信号处理函数代码一开始就分配内存，但是执行栈空间是在收到信号之后才临时在栈上开辟，所以当栈溢出时内核无法在原栈上执行信号处理函数，会导致进程直接终止，sigaltstack()允许进程定义一个备选信号栈，主栈不够用的时候内核在这个备选栈上执行信号处理函数。必须在sigaltstack()中设置SA_ONSTACK标志，才能让指定信号的处理函数使用备选栈
    #include <signal.h>
    #include <stdio.h>
    #include <unistd.h>
    #include <stdlib.h>
    static char alt_stack[SIGSTKSZ];定义备选栈，SIGSTKSZ是系统定义的备选栈默认大小
    void handle_sigsegv(int sig){
        _exit(1);用_exit()安全退出，不刷新缓冲区，不执行atexit回调
    }
    int main(){
        初始化备选栈结构体
        stack_t ss={    
            .ss_sp=alt_stack;   备选栈的起始地址
            .ss_size=sizeof(alt_stack); 备选栈大小
            .ss_flags=0;    0表示启用备选栈，SS_DISABLE表示禁用
        };
        if(sigaltstack(&ss,NULL)==-1){  告诉内核使用这个备选栈
            perror("sigaltstack");
            return 1;
        }
        struct sigaction sa;    配置信号处理规则
        sigemptyset(&sa.sa_mask);
        sa.sa_handle=handle_sigsegv;
        sa.sa_flags=SA_ONSTACK; 核心标志：告诉内核这个信号的处理函数在备选栈执行
        if(sigaction(SIGSEGV,&sa,NULL)==-1){    注册SIGSSEGV信号的处理规则
            perror("sigaction");
            return 1;
        }
        char*ptr=NULL;  故意触发段错误
        *ptr='a';
        return 0;   永远执行不到这里
    }
4.SA_SIGINFO标志
在sigaction的sa_flags中设置SA_SIGINFO标志，提供额外信息，如信号来源、发送进程的PID、UID
void *sa_sigaction (int sig,siginfo_t *info,void *ucontext);
结构体siginfo_t结构体包含了信号的详细上下文
5.系统调用的中断和重启
当一个进程在执行慢速系统调用时收到信号，系统调用会被中断，并返回-1,errno被设置成EINTR，这时在sigaction中设置SA_RESTART标志让中断的系统调用自动重启而不是返回EINTR
比如：你正在加载网页（read() 阻塞），突然断了一下网（触发 SIGINT 信号），如果开了 SA_RESTART（浏览器默认 “自动重试”），网恢复后浏览器会自动重新加载页面；如果没开，浏览器直接提示 “加载失败”，不会重试