//字节序转换
#include <stdio.h>
#include <arpa/inet.h>
int main(){
    uint16_t port = 8080;
    uint32_t ip = 0xC0A80101;//192.168.1.1
    uint16_t net_port = htons(port);
    uint32_t net_ip = htonl(ip);
    printf("Host port:0x%04X,Net port:0x%04X\n", port, net_port);
    printf("Host IP:0x%08X,Net IP:0x%08X\n", ip, net_ip);
    uint16_t host_port = ntohs(net_port);
    uint32_t host_ip = ntohl(net_ip);
    return 0;
}
//IP地址格式转换
#include <stdio.h>
#include <arpa/inet.h>
int main(){
    const char* ip_str = "192.168.1.100";
    struct in_addr ip_bin;
    if(inet_pton(AF_INET,ip_str,&ip_bin)==1){
        printf("Binary IP:0x%08X\n", ip_bin.s_addr);
    }
    char ip_buf[INET_ADDRSTRLEN];
    if(inet_ntop(AF_INET,&ip_bin,ip_buf,sizeof(ip_buf))!=NULL){
        printf("string IP:%s\n", ip_buf);
    }
    return 0;
}
//UDP服务器
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUF_SIZE 1024

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(sockfd);
        return 1;
    }
    printf("UDP server listening on port %d...\n", PORT);

    char buf[BUF_SIZE];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (1) {
        ssize_t n = recvfrom(sockfd, buf, BUF_SIZE - 1, 0,
                             (struct sockaddr*)&client_addr, &client_len);
        if (n > 0) {
            buf[n] = '\0';
            printf("Received: %s\n", buf);
            sendto(sockfd, buf, n, 0, (struct sockaddr*)&client_addr,
                   client_len);
        }
    }
    close(sockfd);
    return 0;
}
//UDP客户端
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUF_SIZE 1024

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    const char* msg = "Hello UDP!";
    sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr*)&server_addr,
           sizeof(server_addr));

    char buf[BUF_SIZE];
    struct sockaddr_in src_addr;
    socklen_t src_len = sizeof(src_addr);
    ssize_t n = recvfrom(sockfd, buf, BUF_SIZE - 1, 0,
                         (struct sockaddr*)&src_addr, &src_len);
    if (n > 0) {
        buf[n] = '\0';
        printf("Server echo: %s\n", buf);
    }
    close(sockfd);
    return 0;
}
//getaddrinfo()解析域名
//getaddrinfo()返回的不是一个地址，而是地址链表，它可能同时有IPV4地址和IPV6地址或者有多个IP(网站做了负载均衡，多台服务器对应同一个域名)，getaddrinfo()会把所有这些地址整理成单向链表
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
// int main(){
//     struct addrinfo hints = {0};
//     struct addrinfo *res, *p;
//     hints.ai_family = AF_UNSPEC;
//     hints.ai_socktype = SOCK_STREAM;
//     int satus = getaddrinfo("www.example.com", "http", &hints, &res);
//     if(status!=0){
//         printf("%s\n", gai_strerror(status));
//         return 1;
//     }
//     for (p = res; p != NULL; p = p->ai_next) {
//         char ipstr[INET6_ADDRSTRLEN];
//         void* addr;
//         if (p->ai_family == AF_INET) {
//             struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
//             addr = &(ipv4->sin_addr);
//             inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
//             printf("IPv4: %s\n", ipstr);
//         } else if (p->ai_family == AF_INET6) {
//             struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)p->ai_addr;
//             addr = &(ipv6->sin6_addr);
//             inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
//             printf("IPv6: %s\n", ipstr);
//         }
//     }
//     freeaddrinfo(res);
//     return 0;
// }
//TCP服务器
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUF_SIZE 1024

int main() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 5) == -1) {
        perror("listen");
        close(listen_fd);
        return 1;
    }
    printf("TCP server listening on port %d...\n", PORT);

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int conn_fd =
        accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
    if (conn_fd == -1) {
        perror("accept");
        close(listen_fd);
        return 1;
    }

    char buf[BUF_SIZE];
    ssize_t n = read(conn_fd, buf, BUF_SIZE - 1);
    if (n > 0) {
        buf[n] = '\0';
        printf("Received: %s\n", buf);
        write(conn_fd, buf, n);
    }
    close(conn_fd);
    close(listen_fd);
    return 0;
}
//TCP客户端
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define BUF_SIZE 1024

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) ==
        -1) {
        perror("connect");
        close(sockfd);
        return 1;
    }

    const char* msg = "Hello TCP!";
    write(sockfd, msg, strlen(msg));

    char buf[BUF_SIZE];
    ssize_t n = read(sockfd, buf, BUF_SIZE - 1);
    if (n > 0) {
        buf[n] = '\0';
        printf("Server echo: %s\n", buf);
    }
    close(sockfd);
    return 0;
}
