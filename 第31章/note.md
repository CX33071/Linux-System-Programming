1.线程安全：一个函数在多线程环境下被并发调用时，总能产生正确的结果，且不会出现数据竞争或状态混乱
  可重入性：函数可以在执行过程中被中断，然后在中断返回后继续执行而不产生错误。可重入函数一定是线程安全的，但线程安全函数不一定是可重入的（例如使用了互斥锁的函数）
  导致线程不安全的常见原因：
    使用全局或静态变量：多个线程同时修改这些变量会导致数据竞争。
    调用了非线程安全的函数：例如标准 C 库中的 strtok()、rand() 等。
    持有状态：函数内部维护了依赖于调用历史的状态
静态缓冲区=线程不安全？
错误，只是当很多线程一起写同一个静态缓冲区的时候会产生竞争，num++对应3条CPU指令
读：把值从内存读到 CPU 寄存器；
改：寄存器里的值 + 1；
写：把寄存器的值写回内存
现在有2个线程num=0,num++,可能第一个线程改了但是还没写入内存，这是第二个线程执行完，num=1，第1个线程写入num=1，实际想要num=2,但是现在结果是1
线程执行函数：char *unsafe_strcat(const char *a, const char *b) {
    static char buf[1024]; // 静态变量，所有线程共享
    snprintf(buf, sizeof(buf), "%s%s", a, b);
    return buf;
}
2.一次性初始化
是 POSIX 线程中用于保证某个初始化函数只被执行一次的核心函数，哪怕多个线程同时调用它，也能确保初始化逻辑仅执行一次。这个函数特别适合解决 “多线程环境下的单例初始化”“全局资源只初始化一次” 这类问题。由第一个调用的线程执行，其他线程会阻塞等待初始化完成，给定一个 pthread_once_t 类型的控制变量
#include <pthread.h>
// 控制变量类型，必须用 PTHREAD_ONCE_INIT 静态初始化
pthread_once_t once_control = PTHREAD_ONCE_INIT;
// 函数原型：成功返回 0，失败返回非 0 错误码
int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));写在线程执行函数中
once_control：全局 / 静态的控制变量，标记初始化是否完成；
init_routine：要保证只执行一次的初始化函数（无参数、无返回值）
3.线程特有数据
许多旧的库函数（如 strtok()）使用静态变量来保存状态，导致它们在多线程环境下不安全。
解决方案：使用线程特有数据（Thread-Specific Data, TSD），为每个线程维护一份独立的数据副本。这样，函数看起来是 “全局” 的，但实际上每个线程操作的是自己的数据
特有数据API：
// 创建一个键，并可选地注册一个析构函数
int pthread_key_create(pthread_key_t *key, void (*destructor)(void*));
// 将线程特定数据与键绑定
int pthread_setspecific(pthread_key_t key, const void *value);
// 获取与键绑定的线程特定数据
void *pthread_getspecific(pthread_key_t key);
// 销毁键
int pthread_key_delete(pthread_key_t key);
线程特有数据的实现限制：
    每个进程可创建的键的数量有限（通常是 1024 个，由 PTHREAD_KEYS_MAX 定义）。
    析构函数的调用顺序是不确定的，不要依赖特定的顺序
4.线程局部存储
是一种更现代、更便捷的线程特有数据实现方式，由编译器和链接器直接支持。
语法：在 C 语言中，使用 __thread 或 thread_local（C11 标准）关键字修饰变量，即可使其成为线程局部的。
static __thread int errno; // 每个线程都有自己的 errno  头文件后
    语法简洁，无需手动管理键和内存分配。
    访问速度比 TSD 更快。
    只能用于静态或全局变量，不能用于动态分配的内存
5.总结
线程安全：核心在于避免多线程对共享状态的并发修改。可以通过以下方式实现：
    无状态：函数不使用任何全局或静态变量。
    线程特有数据（TSD/TLS）：为每个线程提供独立的数据副本。
    同步机制：使用互斥量等锁机制保护共享数据的访问。
线程特有数据（TSD）：通过 pthread_key_* API 实现，灵活但需要手动管理内存。
线程局部存储（TLS）：通过 __thread 关键字实现，简洁高效，是现代 C/C++ 多线程编程的首选方案。
一次性初始化：使用 pthread_once() 确保关键资源只被初始化一次，是实现线程安全库的重要工具。