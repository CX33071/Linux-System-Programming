1.流式套接字上的部分读和部分写
TCP是字节流协议，没有消息边界，read()/write()可能只操作部分数据：
    read()：内核缓冲区数据不足时，返回值小于请求长度。
    write()：内核缓冲区满时，返回值小于请求长度（非阻塞模式更常见）。
解决方案：循环读写，必须用循环确保完整读写，直到数据传输完毕
2.shutdown()
close() 会完全关闭 Socket 并释放文件描述符，而 shutdown() 可以半关闭连接，只关闭一个方向
#include <sys/socket.h>
int shutdown(int sockfd, int how);
how 参数：
    SHUT_RD：关闭读方向，无法再接收数据。
    SHUT_WR：关闭写方向，无法再发送数据（常用）。
    SHUT_RDWR：关闭读写，等价于 close()
核心场景：
    优雅关闭：客户端发送完数据后调用 shutdown(fd, SHUT_WR)，告知服务端 “我发完了”，然后继续读取服务端的剩余响应。
    避免数据丢失：close() 可能丢弃未发送的缓冲区数据，而 shutdown() 会保证数据发送完毕再关闭
3.专用于套接字的I/O系统调用recv()和send()
#include <sys/socket.h>
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
相比 read()/write()，多了 flags 参数，支持高级操作：
    MSG_PEEK：偷看数据，不清除内核缓冲区。
    MSG_DONTWAIT：非阻塞调用，立即返回。
    MSG_WAITALL：等待直到读取完所有数据（阻塞模式下）。
    MSG_OOB：接收 / 发送带外数据
4.sendfile()
sendfile() 实现零拷贝：直接在文件描述符和Socket 描述符之间传输数据，避免用户态和内核态之间的内存拷贝，性能极高
#include <sys/sendfile.h>
ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count);
in_fd：输入文件描述符（必须支持 mmap()，如普通文件）。
out_fd：输出 Socket 描述符。
offset：文件起始偏移（传 NULL 表示从当前位置开始）
Web 服务器发送静态文件（如 HTML、图片）时，sendfile() 比 read()+write() 快数倍
read ()+write () 完整流向（一句话总结）
磁盘文件(①) → 内核页缓存(②) → 用户缓冲区(③) → 内核Socket缓冲区(④) → 客户端(⑤)核心：read() 是「读进③」，write() 是「从③写出」，③是必经之路。
sendfile ()：数据「不走」用户缓冲区（无③，直接内核内流转）
sendfile() 是「一步到位」，核心是数据全程在内核态流转，完全不碰用户缓冲区③
位置	通俗解释	技术名称
① 磁盘文件	服务器本地的静态文件（如 index.html、logo.png）	磁盘存储 / 文件系统
② 内核页缓存	操作系统内核为文件缓存开辟的内存（读取文件时先放这）	Kernel Page Cache
③ 用户缓冲区	应用程序（Web 服务器）自己的内存空间（代码里定义的 buf）	User Buffer（进程内存）
④ 内核 Socket 缓冲区	操作系统内核为网络发送开辟的内存（数据发往网卡前先放这）	Kernel Socket Buffer
⑤ 客户端	浏览器 / 客户端程序（最终接收数据的地方）	Client Socket
5.获取套接字地址 
    getsockname()：获取本地Socket 地址（自己的 IP + 端口）。
    getpeername()：获取对端Socket 地址（对方的 IP + 端口）。
#include <sys/socket.h>
int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
6.深入探讨 TCP 协议 
1. TCP 报文格式
TCP 报文段（Segment）包含：
    源端口 / 目标端口：标识进程。
    序列号 / 确认号：保证可靠传输。
    标志位：SYN（建立连接）、ACK（确认）、FIN（关闭连接）、RST（重置）、PSH（推送）、URG（紧急）。
    窗口大小：流量控制。
    校验和：数据完整性。
2. TCP 序列号和确认机制
    序列号（SEQ）：标识当前报文段的第一个字节的编号。
    确认号（ACK）：期望收到的下一个字节的编号，告知对端 “我已收到前 N 个字节”。
    滑动窗口：通过窗口大小控制发送速率，避免接收方缓冲区溢出。
3. TCP 状态机与状态迁移
TCP 连接有 11 种状态，核心迁移：
    客户端：CLOSED → SYN_SENT → ESTABLISHED
    服务端：CLOSED → LISTEN → SYN_RCVD → ESTABLISHED
    关闭时：ESTABLISHED → FIN_WAIT1 → FIN_WAIT2 → TIME_WAIT → CLOSED
4. TCP 连接的建立（三次握手）
    客户端发送 SYN，进入 SYN_SENT。
    服务端回复 SYN+ACK，进入 SYN_RCVD。
    客户端回复 ACK，双方进入 ESTABLISHED。
5. TCP 连接的终止（四次挥手）
    主动关闭方发送 FIN，进入 FIN_WAIT1。
    被动关闭方回复 ACK，进入 CLOSE_WAIT。
    被动关闭方发送 FIN，进入 LAST_ACK。
    主动关闭方回复 ACK，进入 TIME_WAIT，等待 2MSL 后关闭。
6. 在 TCP 套接字上调用 shutdown()
shutdown(fd, SHUT_WR) 会发送 FIN 包，触发四次挥手，但保留文件描述符，仍可读取对端数据。
7. TIME_WAIT 状态
    作用：确保最后一个 ACK 包被对端收到，避免旧连接的报文干扰新连接。
    持续时间：2MSL（最长报文段寿命），通常为 1~2 分钟。
    问题：大量短连接会导致 TIME_WAIT 过多，占用端口资源。
7.监视套接字netstat
netstat -tulnp  # 查看所有监听的TCP/UDP端口及进程
netstat -an     # 查看所有连接状态
netstat -s      # 查看TCP/UDP统计信息
-t：TCP
-u：UDP
-l：监听状态
-n：数字形式显示 IP / 端口
-p：显示进程名 / PID
8.使用 tcpdump 来监视 TCP 流量
 看你自己写的程序到底在网络上 “收发了什么”
 你会看到：

    谁连接你
    发了什么数据
    你回了什么
    连接怎么建立、怎么断开

# 捕获指定端口的TCP流量
tcpdump -i any port 8080 -vv
# 捕获指定主机的流量
tcpdump host 192.168.1.100
# 保存到文件，用Wireshark分析
tcpdump -w capture.pcap port 8080
-i any：监听所有网卡。
-vv：详细输出。
-w：保存为 pcap 文件，可在 Wireshark 中可视化分析
9.套接字选项 
是网络编程中定制 socket 行为、优化性能、解决问题的核心工具 —— 简单说，默认的 socket 行为不一定满足你的需求，通过这些选项可以「微调」socket 的工作方式，适配不同场景（比如你写的 echo 服务器、Web 服务器）
Socket 选项通过 setsockopt()/getsockopt() 配置，分为多个层级：
    SOL_SOCKET：通用 Socket 选项。
    IPPROTO_TCP：TCP 特有选项。
    IPPROTO_IP：IP 层选项
#include <sys/socket.h>
int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
10.SO_REUSEADDR 套接字选项 
就是端口复用
核心作用
允许快速重启服务器，避免 “Address already in use” 错误：
    服务器重启时，旧连接还在 TIME_WAIT 状态，端口被占用。
    设置 SO_REUSEADDR 后，新进程可以绑定到同一端口。
11.在 accept() 中继承标记和选项 
新接受的连接 Socket 会继承监听 Socket 的部分选项，如：
    O_NONBLOCK（非阻塞）
    SO_KEEPALIVE（TCP 保活）
    SO_RCVBUF/SO_SNDBUF（接收 / 发送缓冲区大小）
12.TCP vs UDP 
特性	TCP	UDP
连接	面向连接	无连接
可靠性	可靠（重传、确认）	不可靠（尽力而为）
顺序	保证顺序	无序
边界	字节流，无边界	报文，有边界
开销	大（头部 20 字节 + 选项）	小（头部 8 字节）
场景	HTTP/HTTPS、SSH、文件传输	直播、游戏、DNS、广播
13.高级功能 
1. 带外数据（OOB）
    用于紧急数据传输，绕过正常数据流。
    发送：send(fd, "X", 1, MSG_OOB)
    接收：recv(fd, buf, size, MSG_OOB)
    场景：远程终端的中断信号（如 Ctrl+C）。
2. sendmsg() 和 recvmsg()
最通用的 I/O 函数，支持分散 / 聚集 I/O（Scatter/Gather），一次读写多个缓冲区，适合高性能场景。
3. 传递文件描述符
通过 Unix Domain Socket 传递文件描述符，实现进程间资源共享（如 Web 服务器传递客户端 Socket 给工作进程）。
4. 接收发送端的凭据
通过 SO_PEERCRED 选项获取对端进程的 UID/GID/PID，用于身份验证。
6. SCTP 以及 DCCP 传输层协议
    SCTP：面向消息的可靠传输，支持多宿主、多流，适合电信领域。
    DCCP：不可靠但拥塞控制的传输协议，适合流媒体
14.总结
    流式 Socket I/O：必须处理部分读写，用循环保证数据完整。
    高级 API：shutdown() 实现半关闭，sendfile() 实现零拷贝，recv()/send() 支持高级标记。
    TCP 深度：三次握手、四次挥手、状态机、TIME_WAIT 是理解 TCP 的核心。
    Socket 选项：SO_REUSEADDR 是服务器开发必备，解决端口复用问题。
    高级功能：带外数据、sendmsg()、文件描述符传递等，用于构建复杂网络服务