1。取消线程
想要用pthread cancle是因为想要现在就终止线程但是pthread cancle规定必须要等到一个取消点才能终止
允许一个线程向另一个线程发送终止请求，让目标在合适时机退出
int pthread_cancle(pthread_t thread)只是向目标线程发送一个取消请求，线程不会立即终止，而是在到达取消点时才响应，避免线程突然退出，保证退出前能完成资源清理释放锁、关闭文件、释放内存，线程响应请求时会执行预设的清理函数pthread_cleanup_push,释放资源后再终止
- 取消点
    线程执行过程中检查是否有挂起取消请求的位置
    常见取消点：pthread_join()、pthread_cond_wait()、sleep()、read()、write()等可能阻塞的系统调用
    可以手动创建取消点：如果线程执行的是纯计算循环，没有阻塞调用，可以使用pthread_testcancel()手动插入一个取消点
2.取消状态及类型
通过pthread_setcancelstate()和pthread_setcanceltype()控制响应取消请求的方式
取消状态：PTHREAD_CANCEL_ENABLE(默认)线程可以被取消
        PTHREAD_CANCEL_DISABLE线程忽略所有取消请求，直到状态恢复为ENABLE
取消类型：PTHREAD_CANCEL_DEFERRED(默认)取消请求被延迟，直到线程达到下一个取消点
        PTHREAD_CANCEL_ASYNCHRONOUS线程可以在任何时候被立即取消，甚至在执行指令的时候，风险高
3.线程可取消性的检测
线程可以通过 pthread_setcancelstate() 和 pthread_setcanceltype() 的旧值参数来检测当前的可取消性设置。
通常用于在执行关键代码前保存旧状态，执行后恢复，确保代码的健壮性
4.如果线程在持有互斥锁或分配了内存时被取消，会导致资源泄漏（死锁、内存泄漏）。
解决方案：使用清理函数（Cleanup Handler），在线程被取消或正常退出时自动执行，释放资源
#include <pthread.h>
void pthread_cleanup_push(void (*routine)(void*), void *arg);
void pthread_cleanup_pop(int execute);
routine：清理函数指针
arg：传给清理函数的参数
execute：
    非 0：立即执行该清理函数
    0：只弹出、不执行清理函数，只有线程被取消 / 退出时才会执行。
push 和 pop 必须成对出现，且在同一代码块内，它们是宏，底层会用 { ... } 包裹，不能随便拆
线程退出、被取消、调用 pthread_exit 时，自动执行注册的清理函数，程被取消或调用 pthread_exit() 时，所有已 push 但未 pop 的清理函数会按逆序执行。
以下情况会自动触发所有清理函数：
    线程调用 pthread_exit()
    线程被 pthread_cancel() 取消
    线程从启动函数 return 返回（某些实现等同 pthread_exit）
    sleep(2);
    // 这行根本跑不到，因为取消线程，自动调用清理函数退出，这行存在只是为了编译能过
    pthread_cleanup_pop(0);
线程退出 / 被取消时，只要清理函数还没被 pop 弹出（还在清理栈里），就一定会执行，不管pop几
6.异步取消
线程可以在任何时候被立即终止，无需等待取消点。
风险：线程可能在执行任何指令（甚至是修改共享数据的中间步骤）时被终止，极易导致数据结构损坏、死锁或资源泄漏。

