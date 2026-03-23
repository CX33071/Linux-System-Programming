1.internet domain socket
是基于TCP/IP协议的网络通信端点，用于跨主机进程间通信
地址族：AF_INET:IPv4协议(32位地址)  AP_INET6:IPv6协议(128位地址)
Socket类型：SOCK_STREAM面向连接的可靠字节流TCP
           SOCK_DGRAM无连接的不可靠数据报UDP
本质：和UNIX domain socket接口完全一致，只是地址结构和底层传输协议不同
所有网络编程API:socket()、bind()、connect()对internet domain和UNIX socket是通用的，差异仅在地址结构
网络通信必须处理字节序，地址格式转换，DNS解析等问题
2.网络字节序
字节序：多字节数据在内存中的存储顺序，分为大端序(高位字节存低地址)，小端序(低位字节存低地址)
网络字节序：TCP/IP协议规定必须使用大端序，不同主机间通信时必须同一字节序
转换函数：
#include <arpa/inet.h>
uint16_t htons(uint16_t hostshort); // 主机字节序 → 网络字节序（16位，端口）
uint32_t htonl(uint32_t hostlong);  // 主机字节序 → 网络字节序（32位，IP）
uint16_t ntohs(uint16_t netshort); // 网络字节序 → 主机字节序
uint32_t ntohl(uint32_t netlong);  // 网络字节序 → 主机字节序
3.数据表示
网络传输中，所有多字节数据(端口、IP、结构体等)都必须转换为网络字节序，否则会出现乱码或者数据错误
字符串、单字节数据不受字节序影响，可直接传输，但是socket 编程中，struct sockaddr_in 的 sin_addr.s_addr 字段需要的是 32 位二进制整数（网络字节序），而不是 "192.168.1.1" 这种字符串 ，所以需要inet_ntop这种函数
复杂结构体传输时，需手动序列化，如按字节拼接，避免字节序和对齐问题
4.internet socket address
IPv4地址结构：
    #include <netinet/in.h>
struct sockaddr_in {
    sa_family_t    sin_family;   // 地址族：AF_INET
    in_port_t      sin_port;     // 端口号（网络字节序）
    struct in_addr sin_addr;     // IP 地址（网络字节序）
    unsigned char  sin_zero[8];  // 填充字节，对齐到 struct sockaddr
};
struct in_addr {
    uint32_t s_addr; // 32位 IPv4 地址（网络字节序）
};
IPv6地址结构：
    struct sockaddr_in6 {
    sa_family_t     sin6_family;  // 地址族：AF_INET6
    in_port_t       sin6_port;    // 端口号（网络字节序）
    uint32_t        sin6_flowinfo; // 流信息
    struct in6_addr sin6_addr;    // 128位 IPv6 地址
    uint32_t        sin6_scope_id; // 作用域 ID
};
所有具体地址结构都要强转为struct sockaddr*才能传给系统调用，bind、connect
5.主机和服务转换函数概述
主机名<——>IP地址：将人类可读的主机名www.example.com转换为IP地址，或反向转换
服务号<——>端口号：将服务名如http(支撑网页访问的核心协议服务)转换为端口号(80)，或反响转换
现代API：getaddrinfo()/getnameinfo()支持IPv4/IPv6双栈
过时API：inet_aton()/inet_ntoa()/gethostbyname()仅支持IPv4
6.inet_pton()和inet_ntop()
#include <arpa/inet.h>
// 字符串 IP → 网络字节序二进制 IP
int inet_pton(int af, const char *src, void *dst);
// 网络字节序二进制 IP → 字符串 IP
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
参数：af:地址族 src:输入字符串/二进制IP dst:输出二进制IP/字符串缓冲区   size:dst缓冲区(INET_ADDRSTRLEN或INET6_ADDRSTRLEN)
返回值：inet_pton()：成功返回 1，无效地址返回 0，错误返回 -1
inet_ntop()：成功返回 dst 指针，错误返回 NULL
输出格式:0x%08X
        %格式化输出的起始标识
        08补零+宽度，输出宽度固定为8位，不足8位时左边补0,如果是8则补空格
        X以大写十六进制输出整数
    08适用32位整数如IPv4二进制地址
    04适用16位整数如端口号、短整型
8.域名系统DNS
DNS：域名系统，将主机名解析为IP地址，是互联网的电话簿
解析过程：客户端调用getaddrinfo()发起解析请求
        操作系统查询本地DNS缓存或递归DNS服务器
        DNS服务器逐级查询，返回对应IP地址
记录类型：A：IPv4地址
        AAAA：IPv6地址
        CNAME：别名
        MX：邮件服务器
9./etc/services文件
该文件记录了服务名与端口号的映射关系：
http    80/tcp
https   443/tcp
ssh     22/tcp
dns     53/udp
函数getservbyname()/getservbyport()可读取该文件，实现服务名与端口号的转换
10.独立于协议的主机和服务转换
- getaddrinfo()函数
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints,
                struct addrinfo **res);
作用：将主机名/服务名转换为地址结构列表
参数：node主机名或者IP字符串
     service服务号或者端口号
     hints过滤条件（地址族/socket类型）
     res输出参数，指向地址结构链表
返回值：成功返回0,错误码返回非0错误码
- 释放地址列表：freeaddrinfo()
void freeaddrinfo(struct addrinfo *res);
必须调用该函数释放getaddrinfo()分配的内存，避免内存泄漏
- 错误诊断：gai_strerror()
const char *gai_strerror(int errcode);
将getaddrinfo()返回的错误码转换为可读的错误信息
- getnameinfo()反向解析：将二进制地址转换为主机名+服务名
int getnameinfo(const struct sockaddr *sa, socklen_t salen,
                char *host, size_t hostlen,
                char *serv, size_t servlen, int flags);
12.internet domain socket库头文件
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
13.过时的主机和服务转换API
- inet_aton()和inet_ntoa(),仅支持IPV4,不可重入，线程不安全,应使用 inet_pton()/inet_ntop() 替代
- gethostbyname()和gethostbyaddr(),仅支持 IPv4，不可重入，线程不安全，应使用 getaddrinfo()/getnameinfo() 替代
- getservbyname() 和 getservbyport()
    读取 /etc/services 文件，服务名与端口号转换，可用于简单场景
14.UNIX和internet domain socket的区别
通信范围：UNIX 本机进程间       internet   跨主机/跨网络进程间
地址类型：UNIX  文件系统路径/抽象名字       internet    IP地址+端口号
性能：   UNIX   极高，内核传输            internet    网络传输
可靠性：    UNIX    内核保证            internet    依赖TCP/IP协议
代码兼容性：接口一致
15.总结：优先使用 getaddrinfo() 实现 IPv4/IPv6 双栈兼容
避免使用过时 API
严格处理字节序问题，确保跨主机通信正确