1.概念
一个进程可以包含多个线程，所有线程共享进程的内存、文件句柄等资源。
多线程可以让程序 “同时” 执行多个任务，提升运行效率（比如一边下载文件，一边显示下载进度）
为什么要用多线程？
    提高效率：避免单线程 “卡壳” 导致整个程序无响应，核心是在线程执行的函数内部捕获所有异常，让错误被 “隔离” 在单个线程内
    处理并发任务：比如聊天软件同时接收消息、发送消息、显示界面；爬虫程序同时爬取多个网页。
    异步处理：耗时操作（如网络请求、文件读写）放在后台线程执行，不阻塞主线程（比如 GUI 界面不会卡死）
线程相当于让多个函数一起执行，一个线程调用一个函数，线程的本质是 “独立执行流”，一个线程可执行多个函数，只是我们常按 “一个线程一个函数” 设计
是进程内的独立执行流，共享同一地址空间、文件描述符、信号处理等资源，但拥有独立的程序计数器、栈和寄存器
优势：
    线程创建和上下文切换开销远小于进程。
    共享内存，线程间通信更高效（无需 IPC 机制）。
劣势：
    共享资源导致并发问题（数据竞争、死锁），需同步机制。
    一个线程崩溃（如段错误）可能导致整个进程终止
线程相当于进程的一部分，线程和进程都由task_struct表示，区别在于资源共享程度
多线程编程的核心挑战是同步，确保对共享资源的安全访问
2.Pthreads API
提供了一套线程创建、管理和同步的 API
头文件：#include <pthread.h>
需链接线程库，gcc -pthread program.c -o program
3.创建线程
#include <pthread.h>
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine) (void *), void *arg);
thread输出参数，存创建的新线程的ID
*attr是线程属性，NULL代表默认属性
*start_routine是线程执行的函数，参数和返回值都是*void
*arg是传给线程执行的函数的参数
成功返回0，失败妇女会错误码,不在errno里存
pthread_t pthread_self()反悔的是pyhread_t类型，要进行类型砖混，unsigned long
int pthead_join(pthread_t thread,void**reval)阻塞等待某个线程结束，并回收其资源，防止僵尸线程第二个参数传NULL代表不关心线程的返回值，只等待线程结束
线程返回值可能是return完之后线程执行函数返回的值，也可能是pthread_exit主动退出返回的值
记得(void*)强制转换
4.终止进程
pthread_exit(void*retval)主动终止当前线程，可以指定一个线程退出的返回值，就是传给pthread_join的参数
只终止当前线程，子线程不受影响，区别于线程正常return退出
5.线程ID类型    pthread_t
获取自身ID pthread_t pthread_slef()
比较线程ID  int pthread_equal(pthread_t t1,pthread_t t2);相等返回非0,不相等返回0。pthread_t在不同系统中可能是整数或结构体，不能直接用==比较
7.分离线程
创建线程时设置分离属性，告诉内核：不关心这个线程的退出状态，也不用pthread_join等待这个线程，自动回收这个线程的资源
pthread_join和分离线程的区别：一个线程结束后自动回收，一个调用完再回收，pthread_join可以返回第2个参数，创建新线程时不设置属性的话，如果也不调用thread_join会导致僵尸线程
分离线程不能被join
设置分离线程有2种方式：
- 设置创建的属性PTHREAD_CREATE_DETACHED
- 县默认可连接态创建新线程，然后通过int pthread_detach(pthread_t thread)修改成分离态
8.线程属性对象：pthread_attr_t，用于配置线程的栈大小、分离状态、调度策略等。
常用函数：
    pthread_attr_init(pthread_attr_t *attr)：初始化属性对象。
    pthread_attr_destroy(pthread_attr_t *attr)：销毁属性对象。
    pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate)：设置分离状态。
    pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)：设置栈大小。
9.进程和线程区别
特性	        线程	                         进程
资源共享	共享地址空间、文件描述符等	        独立地址空间，资源隔离
通信方式	直接访问共享内存	            需 IPC（管道、消息队列等）
创建开销	小	                                大
上下文切换	快	                                慢
故障隔离	一个线程崩溃可能导致进程终止	    进程崩溃不影响其他进程
适用场景	高并发、IO 密集型任务	            独立、隔离的任务