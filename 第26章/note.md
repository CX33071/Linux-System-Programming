1.孤儿进程和僵尸进程
孤儿进程：父进程先于子进程退出，子进程被init（PID 1）或systemd收养，成为孤儿进程。孤儿进程退出后，init会自动回收其资源。
僵尸进程：子进程先于父进程退出，但父进程未调用wait()系列函数回收其资源，子进程的进程描述符仍保留在系统中，成为僵尸进程（状态为Z）。
危害：僵尸进程占用 PID 和内核资源，大量僵尸进程会导致系统无法创建新进程
2.等待子进程的几个函数
- wait()用于阻塞父进程，直到任意一个子进程终止，并回收其资源(避免僵尸进程),就是父进程用来等子进程死然后收尸的函数，父进程调用wait()后会停在那里不动直到有子进程终止
#include <sys/wait.h>
pid_t wait(int*status)
如果父进程没有子进程wait()直接返回-1,成功返回终止的紫禁城PID
如果子进程在父进程调用wait之前就死了wait会立刻返回回收这个僵尸进程不会阻塞
wait(NULL)表示只回收子进程资源，不关心子进程为什么退出，不保存错误码
wait()时阻塞函数，如果想让父进程问一下就走：有子进程死就收，没有就立刻返回01,不阻塞用waitpid(-1,&status,WNOHANG);
- waitpid()
可以指定等待的子进程，支持非阻塞模式
pid_t waitpid(pid_t pid,int*status,int options);
pid参数：
>0:等待PID为pid的子进程
=0:等待与父进程同组的任意子进程
=-1：等待任意子进程，相当与wait
<-1:等待进程组ID为-pid的任意子进程
options参数：
WNOHANG：非阻塞模式，若无子进程退出，立即返回0
WUNTRACED：阻塞模式，同时捕获子进程退出状态，不仅等待子进程终止，还等待子进程暂停
- 等待状态值
WIFEXITED(status)：子进程正常退出时返回非 0。
WEXITSTATUS(status)：返回子进程的退出状态码（仅当WIFEXITED为真时有效）。
WIFSIGNALED(status)：子进程被信号终止时返回非 0。
WTERMSIG(status)：返回终止子进程的信号编号（仅当WIFSIGNALED为真时有效）。
WIFSTOPPED(status)：子进程被停止时返回非 0。
WSTOPSIG(status)：返回停止子进程的信号编号（仅当WIFSTOPPED为真时有效）
- 从信号处理程序中终止进程
子进程终止时，内核会向父进程发送SIGCHLD信号
信号驱动回收，在信号处理程序中回收所有退出的子进程
- 系统调用waitid()
可以指定等待的事件类型，退出、停止、继续，并返回更详细的siginfo_t结构体
int waitid(idtype_t idtype,id_t id,siginfo_t*infop,int options)
idtype:P_PID按PID   P_PGID按进程组  P_ALL按所有子进程
options：WEXITED（等待退出）、WSTOPPED（等待停止）、WCONTINUED（等待继续）
- wait3()和wait4()
额外返回子进程的资源使用情况
3.SIGCHLD信号
子进程终止、停止、继续时内核可能会给父进程发送SIGCHLD信号，具体发不发取决父进程的配置
默认sa_flags含SA_NOCLDSTOP,只在子进程终止时触发
配置时sa_flags=0,终止/暂停/继续都触发
信号处理程序中应循环调用waitpid(-1,&status,WNOHANG)，回收所有退出的子进程，在主循环进行轮询来回收子进程也可以，但是轮询的本质是主动问，必定会面临响应延迟+CPU消耗的问题
忽略终止的子进程：父进程可显式忽略SIGCHLD信号，signal(SIGCHLD,SIG_IGN),此时内核会自动回收终止的子进程，避免产生僵尸进程
WNOHANG 控制 waitpid 的「调用方式」（非阻塞 vs 阻塞）；
WUNTRACED 控制 waitpid 的「检测范围」（仅终止 vs 终止 + 暂停）
SIGCHLD 是父进程的 “子进程状态通知器”，默认只通知终止，关闭 SA_NOCLDSTOP 可通知暂停 / 继续；
信号处理程序必须用 while(waitpid(-1, &status, WNOHANG)) 循环回收，避免漏收（信号非排队）；
WNOHANG 是刚需（防止处理程序阻塞），WUNTRACED 是可选（仅调试 / 监控需要）