服务器设计
1.迭代型服务器：每次只处理一个客户端，只有当完全处理完一个客户端的请求后采取处理下一个客户端,适用UDP
并发型服务器：能够同时处理多个客户端的请求，适用TCP
迭代型服务器只适用于能够快速处理客户端请求的场景
传统并发型服务器设计思路：多进程：fork()一个子进程处理每个连接
                      多线程：pthread_create()创建一个线程处理每个连接
                      I/O多路复用：select()/poll()/epoll()单进程处理多个连接(高性能)
1.迭代型UDP echo 服务器
UDP 是无连接协议，服务器循环调用 recvfrom() 接收数据，再用 sendto() 原样返回（echo 服务）
echo服务支持UDP和TCP，UDP echo服务器连续读取数据报，将每个数据报的拷贝返回给发送者
- 使用becomeDaemon()函数将服务器转换为一个守护进程
- 如果服务器无法将回复发送给客户端，就使用syslog记录一条日志消息
2.端口复用
为什么需要端口复用？（核心问题场景）
TCP 连接断开后，端口不会立即释放，而是进入 TIME_WAIT 状态（默认约 2 分钟），目的是确保网络中残留的数据包被处理，避免新连接收到旧数据。
如果没有设置 SO_REUSEADDR，会出现以下情况：
    你启动服务器，绑定 8080 端口，正常运行；
    你关闭服务器（或程序崩溃），8080 端口进入 TIME_WAIT 状态；
    你立即重启服务器，调用 bind(8080) 时会报错：Address already in use（地址已被占用）；
    必须等 2 分钟（TIME_WAIT 超时），才能重新绑定该端口。
设置之后： 允许绑定处于 TIME_WAIT 状态的端口；
        允许同一端口被同一主机的多个 socket 绑定（需满足条件）；
        允许重启服务器时快速复用端口
3.并发型服务器的其他设计方案
- 多线程版：
        用线程代替进程，开销更小，共享内存
        // 线程函数：处理单个连接
void *handle_client(void *arg) {
    int conn_fd = *(int*)arg;
    free(arg);
    char buf[BUF_SIZE];
    ssize_t n;
    while ((n = read(conn_fd, buf, BUF_SIZE)) > 0) {
        write(conn_fd, buf, n);
    }
    close(conn_fd);
    return NULL;
}
// 主循环中创建线程
pthread_t tid;
int *pconn = malloc(sizeof(int));
*pconn = conn_fd;
pthread_create(&tid, NULL, handle_client, pconn);
pthread_detach(tid); // 线程分离，自动回收资源
- I/O多路复用
用 select()/poll()/epoll() 监听多个 socket，单进程处理所有连接，适合高并发、低延迟场景
- 预派生子进程 / 线程池
预先创建一组进程 / 线程，避免频繁 fork()/pthread_create() 的开销，适合稳定高并发场景
4.inetd(internet超级服务器)守护进程
inetd：经典的 “超级服务器”，监听多个端口，收到连接后fork() 子进程并执行对应的服务程序
优势：
    统一管理多个服务，避免每个服务都监听端口。
    按需启动服务，节省系统资源
echo    stream  tcp     nowait  root    internal
echo    dgram   udp     wait    root    internal
表示：echo 服务由 inetd 内部处理，TCP 用 nowait（并发），UDP 用 wait（迭代）。
inetd 本质是进程托管，把并发逻辑交给 inetd，业务服务只需要处理标准输入输出。
现代系统中已很少直接使用，但设计思想影响了 systemd 等现代服务管理器。
