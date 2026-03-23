单线程 / 进程同时监控多个文件描述符，当某个描述符就绪（可读 / 可写）时再处理，从而实现高并发。
主要 I/O 模型
    I/O 多路复用：select() / poll() → 阻塞等待多个 fd 就绪。
    信号驱动 I/O：内核在 fd 就绪时发送信号通知进程，异步处理。
    epoll：Linux 特有的高性能 I/O 多路复用，是 select/poll 的升级版。
    非阻塞 I/O：配合以上模型，避免单个 I/O 操作阻塞整个进程。
1.水平触发和边缘触发
    水平触发（Level Triggered, LT）：只要 fd 处于就绪状态，就会持续通知（例如：只要缓冲区有数据，就一直报告可读）。
        select/poll 默认是水平触发，epoll 也支持。
    边缘触发（Edge Triggered, ET）：仅在 fd 状态发生变化时通知一次（例如：缓冲区从空变为有数据时通知一次，之后不再通知，直到数据被读完再次有新数据到来）。1.fd 必须设置为非阻塞。2.循环 read()/recv() 直到返回 -1 且 errno == EAGAIN
        只有 epoll 支持，性能更高，但需要更谨慎的编程。
在备选的 I/O 模型中采用非阻塞 I/O
    所有高效 I/O 模型都建议配合非阻塞 fd使用：
        当 fd 被报告为可读时，read() 可能仍会阻塞（例如数据被内核收回），非阻塞模式下会立即返回 -1 并设置 EAGAIN/EWOULDBLOCK。
        边缘触发模式下，必须循环读取直到 EAGAIN，否则会导致事件丢失
2.I/O多路复用
I/O多路复用支持进程同时阻塞等待多个文件描述符，直到其中任意一个或多个变为就绪状态
- select
#include <sys/select.h>
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
// 辅助宏
void FD_ZERO(fd_set *set);          // 清空集合
void FD_SET(int fd, fd_set *set);   // 添加 fd 到集合
void FD_CLR(int fd, fd_set *set);   // 从集合移除 fd
int  FD_ISSET(int fd, fd_set *set); // 检查 fd 是否在集合中
参数：
    nfds：最大文件描述符值 + 1（select 会遍历 0 到 nfds-1 的所有 fd）。
    readfds/writefds/exceptfds：分别监控可读、可写、异常事件的 fd 集合。
    timeout：超时时间，NULL 表示永久阻塞。
返回值：就绪 fd 的总数，超时返回 0，错误返回 -1
核心缺点:
    最大文件描述符数受限于 FD_SETSIZE（通常为 1024），无法处理大量连接。
    每次调用都需要重新传递所有 fd 集合，内核遍历开销大。
    无法知道具体哪些 fd 就绪，需要遍历所有 fd 检查 FD_ISSET
select 会彻底修改你传入的 fd 集合—— 调用后，集合里只留下就绪的 fd，所有未就绪的 fd 都会被从集合中移除（清空）。
但关键是：select 不会告诉你哪个 fd 就绪了，只会告诉你有几个 fd 就绪了（返回值）。因此你必须重新检查每个 fd 是否还在集合里，才能确定到底是哪个 fd 就绪了
- poll()
#include <poll.h>
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
struct pollfd {
    int   fd;      // 文件描述符
    short events;  // 要监控的事件
    short revents; // 实际发生的事件（内核填充）
};
特性	                    select	                        poll
监听的 fd 数量	    受限于 FD_SETSIZE（默认 1024）	            无上限（只受系统资源限制）
参数形式	         三个独立的 fd 集合（读 / 写 / 异常）	    一个 pollfd 结构体数组（更灵活）
fd 传递方式	        传入后被修改（需每次重新初始化）	        只修改「就绪状态」，无需重新初始化数组
返回值	                    就绪 fd 数量	                         就绪 fd 数量
通俗比喻
    select：你给老师一张「要检查的学生名单」，老师检查后只还给你「做对题的学生」，下次要重新写名单；
    poll：你给老师一张「学生名单 + 要检查的题型」，老师检查后在名单上标注「谁做对了」，名单本身还在，下次只需看标注。
struct pollfd {
    int   fd;         // 要监听的文件描述符（比如 listen_fd、client_fd）
    short events;     // 要关注的事件（比如 POLLIN=读就绪，POLLOUT=写就绪）
    short revents;    // poll 返回后，实际发生的事件（由内核填充，只读）
};
常用事件：
    POLLIN：可读
    POLLOUT：可写
    POLLERR：错误
优势：
    没有 FD_SETSIZE 限制，可监控任意数量的 fd。
    事件与结果分离，无需每次重新构建集合。
    直接通过 revents 知道每个 fd 的就绪事件。
定义 pollfd 数组，初始化要监听的 fd（比如 listen_fd）；
调用 poll 等待事件就绪；
遍历数组，检查每个 fd 的 revents，处理就绪事件；
重复步骤 2-3（无需重新初始化数组，只需更新 revents）
1. 第一个参数：struct pollfd *fds
核心含义
指向 pollfd 结构体数组的指针 ——你要告诉内核「监听哪些 fd + 关注这些 fd 的什么事件」
fd=-1 表示这个位置空闲，内核会跳过，这是标记「不监听」的标准做法；
events 常用取值：POLLIN（读就绪）、POLLOUT（写就绪）、POLLERR（出错）；
revents 是「输出参数」：由内核修改，你只能读，不要手动改
2. 第二个参数：nfds_t nfds
核心含义
pollfd 数组的有效长度—— 告诉内核「要检查数组里前多少个元素」
3. 第三个参数：int timeout
核心含义
poll 的超时时间（毫秒） —— 告诉内核「最多等多久，没就绪事件就返回」
-1	无限等待（阻塞）	
0	立即返回（非阻塞）
4. select() 和 poll() 存在的问题
    O (n) 时间复杂度：每次调用都要遍历所有监控的 fd，当连接数达到上万时，性能急剧下降。
    用户态与内核态数据拷贝：每次调用都要将 fd 集合从用户态拷贝到内核态，开销大。
    无法高效处理大量连接：不适合 C10K 问题（并发 1 万连接）
3.信号驱动I/O
进程预先告知内核：当某个 fd 就绪时，向我发送一个信号（如 SIGIO），进程在信号处理函数中异步处理 I/O，无需阻塞等待
文件描述符何时就绪？
    可读就绪：
        套接字接收缓冲区有数据。
        管道 / 终端有数据可读。
        监听 socket 有新连接。
        对端关闭连接（read() 返回 0）。
    可写就绪：
        套接字发送缓冲区有空闲空间。
        管道 / 终端可写入数据。
    异常就绪：
        套接字收到带外数据（OOB）
普通阻塞 I/O	进程主动调用 read/accept 阻塞等待	fd未就绪时阻塞（不占 CPU）	
普通非阻塞 I/O	进程轮询调用 read/accept 检查	fd未就绪时运行（轮询耗 CPU）	
信号驱动 I/O（SIGIO）	进程注册信号，内核 fd 就绪时发 SIGIO	fd未就绪时空闲（可做其他事）	
4.epoll编程接口
是linux系统特有的高性能I/O多路复用API，解决了select/poll的缺点
- 创建epoll:epoll_create()
#include <sys/epoll.h>
int epoll_create(int size); // size 为提示内核的初始大小，现已无实际意义，填任意正整数即可
返回一个 epoll 实例的文件描述符，后续操作都基于此 fd
- 修改epoll的兴趣列表：epoll_ctl()
给 epoll 添加 / 删除 / 修改要监听的 fd，以及指定要监听该 fd 的哪些事件（比如读就绪、写就绪）
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
第一个参数：int epfd:epoll 实例的文件描述符
第二个参数：int op
    含义：要执行的「操作类型」，只有 3 种合法值，决定对 fd 做什么操作：
    取值	含义	你的服务器场景
    EPOLL_CTL_ADD	把 fd 加入 epoll 实例的监听列表	新客户端连接后，将 client_fd 加入 epoll 监听
    EPOLL_CTL_DEL	把 fd 从 epoll 实例的监听列表中移除	客户端断开连接后，移除对应的 client_fd
    EPOLL_CTL_MOD	修改已监听 fd 的事件类型（比如加写事件）	原本只监听读就绪，现在要同时监听写就绪
第三个参数：int fd，含义：要操作的目标文件描述符
第四个参数：struct epoll_event *event,含义：指定要监听 fd 的「事件类型」，以及 fd 的「关联数据」（比如自定义标识），是 epoll 灵活的核心
struct epoll_event {
    uint32_t events;  // 要监听的事件（比如 EPOLLIN 读就绪）
    epoll_data_t data;// 关联数据（常用 fd，也可以传指针）
};
// 关联数据的联合体（二选一）
typedef union epoll_data {
    void    *ptr;    // 自定义指针（比如指向客户端结构体）
    int      fd;     // 最常用：要监听的 fd 本身
    uint32_t u32;
    uint64_t u64;
} epoll_data_t;
常用 events 取值：
取值	含义	你的服务器场景
EPOLLIN	读就绪（新连接 / 客户端发数据）	监听 listen_fd/client_fd 读事件
EPOLLOUT	写就绪（发送缓冲区有空）	客户端 fd 要批量发数据时监听
EPOLLET	边缘触发（ET）模式	替代默认的水平触发（LT）
EPOLLONESHOT	只监听一次事件，触发后需重新添加	避免多线程重复处理同一个 fd
- epoll_wait()
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
    events：输出数组，内核将就绪事件写入此数组。
    maxevents：数组最大长度，即最多返回多少个就绪事件。
    timeout：超时时间（毫秒），-1 表示永久阻塞。
返回值：就绪事件的数量
- 注意：返回值：成功返回 0，失败返回 -1（错误码存在 errno 中）；
ET 模式必须配非阻塞 fd：如果用 EPOLLET，必须先通过 fcntl 将 fd 设置为非阻塞，否则可能导致数据读取不完整、进程卡死；
EPOLL_CTL_DEL 必须调用：客户端断开后，一定要从 epoll 移除 fd，否则 epoll 会一直监听无效 fd，导致「僵尸监听」
- 必须将文件描述符设置为非阻塞模式：
    fcntl(fd,F_GETFL,0)fcntl 是「文件控制」函数，用来修改 fd 的各种属性；F_GETFL 是「获取文件状态标志」的指令,返回值 flags 是一个整数
    fcntl(fd, F_SETFL, flags | O_NONBLOCK),F_SETFL 是「设置文件状态标志」的指令；O_NONBLOCK 是系统定义的常量（本质是一个二进制位），代表「非阻塞模式」
兴趣列表：内核维护的、进程要监控的 fd 集合，通过 epoll_ctl() 修改。
就绪列表：内核维护的、已经就绪的 fd 集合，epoll_wait() 直接返回此列表，无需遍历所有 fd，时间复杂度 O (1)
特性	select/poll	epoll
时间复杂度	O(n)	O (k)（k 为就绪 fd 数）
数据拷贝	每次调用都拷贝所有 fd	仅在 epoll_ctl() 时拷贝
最大连接数	受限于 FD_SETSIZE	无限制
触发模式	仅水平触发	水平触发 + 边缘触发
适用场景	少量连接	海量连接（C10K+）
5.当程序需要同时等待信号和文件描述符时，可能会出现竞态条件：
(1)「先调用 select/poll 等 fd 就绪，同时注册信号处理函数，希望信号来的时候打断 select/poll」。但信号可能在「select/poll 调用前」就到达，导致 select/poll 被永久阻塞（因为信号已经处理完，后续没有信号能打断它）
(2)「先检查 fd 是否就绪，再调用 pause () 等信号，希望 fd 就绪时能被信号打断 pause」。但 fd 可能在「检查 fd」和「调用 pause」之间就绪，导致 pause 被永久阻塞，且 fd 就绪事件被错过。
本质是「检查事件」和「进入等待」不是原子操作—— 中间有一个「时间窗口」，事件刚好在这个窗口发生，导致：
    信号提前到 → 等待 fd 时没信号打断，永久阻塞；
    fd 提前到 → 等待信号时没 fd 事件触发，错过 fd
- pselect()
int pselect(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
            const struct timespec *timeout, const sigset_t *sigmask);
原理：pselect 的 sigmask 参数把「修改信号掩码 + 等待 fd / 信号」做成了原子操作，彻底消除时间窗口：
    调用 pselect 前，你可以屏蔽所有关注的信号（比如 SIGUSR1），避免信号在「检查 - 等待」期间到达；
    调用 pselect 时，内核原子化地「解除指定信号的屏蔽 + 进入等待」；
    只要 fd 就绪 或 指定信号到达，pselect 就会返回；
    返回后，内核自动恢复原来的信号掩码，避免后续代码被信号打断
- self-pipe技巧
原理：创建一个管道，在信号处理函数中向管道写一个字节，主循环中用 select()/poll()/epoll() 监控管道读端，将信号转换为 I/O 事件
6.总结：I/O 多路复用：select/poll 是 POSIX 标准，适合少量连接；epoll 是 Linux 高性能方案，适合海量连接。
信号驱动 I/O：异步通知，但信号不可靠，需配合 self-pipe 等技巧