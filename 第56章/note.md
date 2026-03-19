1.socket
可以把 socket 理解成：操作系统给程序员提供的 “网络通信工具箱”,是操作系统内核实现的网络通信底层机制
domain
Domain（域）不是一套统一标准，
但它是【很多通信标准里都在用的通用设计机制】,不同通信标准（DDS、SOME/IP、5G、车载以太网、DNS…）都用了 Domain 这个设计思路：分组隔离寻址权限
    流 Socket（SOCK_STREAM）：面向连接、可靠、有序、无重复的字节流服务（对应 TCP）。
    数据报 Socket（SOCK_DGRAM）：无连接、不可靠、无序、有最大长度限制的报文服务（对应 UDP）。
    地址族domain：
    AF_INET：IPv4 网络协议。
    AF_INET6：IPv6 网络协议。
    AF_UNIX（或 AF_LOCAL）：本地进程间通信（Unix Domain Socket）
2.创建一个socket
#include <sys/socket.h>
int socket(int domain, int type, int protocol);
domain：地址族，如 AF_INET（IPv4）。
type：Socket 类型，如 SOCK_STREAM（TCP）流、SOCK_DGRAM（UDP）数据报。
protocol：具体协议，通常填 0 让系统自动选择对应协议
    返回值：成功返回文件描述符（fd），失败返回 -1 并设置 errno。
    Socket 本质是一个文件描述符，可以用 read()/write() 等 I/O 函数操作。
    每个 Socket 在内核中对应一个缓冲区（发送 / 接收队列）
3.将socket绑定到地址bind()
#include <sys/socket.h>
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    将 Socket 与特定 IP 地址 + 端口号绑定，让客户端能找到这个服务。相当于注册一个电话号码
    参数说明：
        sockfd：socket() 返回的文件描述符。
        addr：指向地址结构的指针（如 struct sockaddr_in）。
        addrlen：地址结构的大小（用 sizeof() 计算）。
    返回值：成功返回 0，失败返回 -1。
    端口号规则：
        1~1023：知名端口（需 root 权限）。
        1024~49151：注册端口。
        49152~65535：临时 / 动态端口。
    通配地址 INADDR_ANY：表示监听本机所有网卡的 IP 地址。
    字节序问题：必须用 htons()/htonl() 将主机字节序转为网络字节序（大端）
4.通用 socket 地址结构：struct sockaddr
struct sockaddr {
    sa_family_t sa_family;  // 地址族（如 AF_INET）
    char        sa_data[14]; // 地址数据（IP+端口）
};
sa_data 是通用字段，不同地址族格式不同，不方便直接操作。
实际使用：用具体地址结构（如 struct sockaddr_in for IPv4），然后强转为 struct sockaddr* 传给系统调用。
IPv4专用结构：struct sockaddr_in {
    sa_family_t    sin_family;   // 地址族 AF_INET
    in_port_t      sin_port;     // 端口号（网络字节序）
    struct in_addr sin_addr;     // IP 地址
    unsigned char  sin_zero[8];  // 填充字节，与 struct sockaddr 对齐
};

struct in_addr {
    uint32_t s_addr; // 32 位 IPv4 地址（网络字节序）
};
强转必要性：bind()/connect() 等函数只接受 struct sockaddr*，所以必须强转。
地址转换：用 inet_pton() 将字符串 IP（如 "127.0.0.1"）转为网络字节序，inet_ntop() 反向转换
5.流socket(TCP)
流 Socket 是面向连接的可靠服务，对应 TCP 协议，
流程为：server: socket() → bind() → listen() → accept() → read()/write() → close()
        client: socket() → connect() → write()/read() → close()
- 监听接入连接：listen()
#include <sys/socket.h>
int listen(int sockfd, int backlog);
    作用：将 Socket 转为被动模式，等待客户端连接。
    参数：
        backlog：等待连接队列的最大长度（半连接队列 + 全连接队列）。
    返回值：成功 0，失败 -1。
    调用 listen() 后，Socket 进入监听状态，只能接受连接，不能直接收发数据
- 接受连接：accept()
#include <sys/socket.h>
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
    作用：从等待队列中取出一个客户端连接，返回新的连接 Socket（用于和该客户端通信），回新创建一个socket。
    参数：
        addr：输出参数，保存客户端地址信息（可填 NULL 不关心）。
        addrlen：输入输出参数，传入地址结构大小，返回实际大小。
    返回值：成功返回新连接的 fd，失败 -1。
    accept() 是阻塞调用：没有连接时会一直等待。
    原监听 Socket（sockfd）继续监听新连接，新连接 Socket（返回值）负责数据传输
- 连接到对等socket:connect()
#include <sys/socket.h>
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
    作用：客户端主动向服务器发起连接（TCP 三次握手）。
    参数：addr 是服务器的地址结构（IP + 端口）。
    返回值：成功 0，失败 -1。
    客户端无需 bind()，系统会自动分配临时端口。
    若连接失败，需关闭 Socket 后重新创建
- 流socket I/O
流 Socket 是字节流，没有消息边界，read()/write() 与普通文件 I/O 一致：
    read()：从接收缓冲区读取数据，可能返回少于请求的字节数（需循环读取）。
    write()：向发送缓冲区写入数据，可能阻塞（缓冲区满时）。
    常用替代函数：send()/recv()
- 连接终止：close ()
    调用 close(fd) 会关闭 Socket，触发 TCP 四次挥手。
    半关闭：用 shutdown(fd, SHUT_WR) 只关闭写方向，仍可读取对方数据
6.数据报
数据报 Socket 是无连接的不可靠服务，对应 UDP 协议，无需连接，直接收发报文。
- 交换数据报：recvfrom () 和 sendto ()
#include <sys/socket.h>
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                const struct sockaddr *dest_addr, socklen_t addrlen);
recvfrom()：接收数据报，并获取发送方的地址。
sendto()：向指定地址发送数据报。
参数：flags 通常填 0，src_addr/dest_addr 保存对端地址
- 在数据报 socket 上使用 connect ()
- 对 UDP Socket 调用 connect() 只是绑定默认对端地址，不会发起网络连接。
之后可以直接用 read()/write() 收发数据，无需每次指定地址。
优点：简化代码，内核会过滤非目标地址的报文
recvfrom和sendto是 UDP（数据报套接字）的核心收发函数 ——recvfrom用于接收 UDP 数据包并获取发送方的地址，sendto用于向指定地址发送 UDP 数据包；两者的核心价值是解决 UDP “无连接” 特性下的 “定向收发” 问题
7.客户端是如何通过socket找到服务的
客户端通过「IP 地址 + 端口号」这两个核心标识，结合 TCP/UDP 协议的通信规则，在网络中定位并连接到目标服务端，整个过程就像你打电话时输入对方的电话号码（IP + 端口）并拨通一样
8.客户端调用connect()后的阻塞，不是直接 “等服务器调用 accept ()”，而是：
    客户端connect()的阻塞，本质是等「TCP 连接从服务器内核的 “全连接队列” 中被取出」—— 而accept()的作用就是从这个队列里取连接，所以表象上看，客户端确实是等服务器调用accept()后才结束阻塞。
9.“已被连接的 socket” 分两种：一种是「监听 socket」（用于 listen/accept），一种是「通信 socket」（用于和客户端收发数据），前者始终保持打开并接受新连接，后者是专属连接，两者不是同一个 socket
10.数据报 socket（SOCK_DGRAM）本质是无连接的，sendto/recvfrom 只是 “指定目标地址收发数据”，并没有建立像 TCP 那样的 “持久连接”，它们的作用不是 “连接”，而是 “定向收发数据”,只是单次数据收发时指定目标地址，内核不维护任何连接状态，每次收发都是独立的,无会话状态，每次 sendto 都要手动指定目标 IP + 端口，每次 recvfrom 都要获取发送方地址,TCP 的 “连接” 是协议层持久、有状态的双向会话，read/write基于这个会话收发字节流；UDP 即便能用read/write，也只是内核记录了默认目标地址的 “伪连接”，read/write本质还是无状态的数据包收发
11.需要可靠、有序、无边界字节传输的场景用 TCP 流；需要高效、低延迟、按完整数据包传输的场景用 UDP 数据报
    流socket:网页浏览、文件传输、实时通讯的文字聊天如微信、QQ、
    数据报socket:音视频通话、抖音直播、网络游戏王者和平、DNS域名解析
12.对比：UDP 的两种发送方式
    未调用connect：必须用sendto(sfd, data, len, 0, &servaddr, sizeof(servaddr))，显式指定服务端地址；
    调用connect后：直接用write(sfd, data, len)，数据会自动发往connect绑定的服务端地址。
13.inet_ntop(AF_INET, &sa->sin_addr, ip, sizeof(ip));这步是干什么的
把 sa 指向的 sockaddr_in 结构体里的二进制 IPv4 地址（比如 0x0100007F），转换成如 "127.0.0.1" 这样的字符串，并存到 ip 缓冲区中。
14.地址结构
基类    struct sockaddr
UNIX    struct sockaddr_un
IPv4    struct sockaddr_in
IPv6    struct sockaddr_in6
通用    struct sockaddr_storage
