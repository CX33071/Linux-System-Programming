//创建TCP socket
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
int main(){
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd==-1){
        perror("socket failed");
        return 1;
    }
    printf("socket created successfully");
    close(socketfd);
    return 0;
}
//绑定到8080端口
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
int main(){
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if(socketfd==-1){
        perror("socket create failed");
        return 1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;//IPv4
    addr.sin_port = htons(8080);//端口8080,转网络字节序,htonl用于IPV4地址转网络字节序
    addr.sin_addr.s_addr = INADDR_ANY;//IP地址，监听所有网卡
    if(bind(socketfd,(struct sockaddr*)&addr,sizeof(addr))==-1){
        perror("bind failed");
        close(socketfd);
        return 1;
    }
    close(socketfd);
    return 0;
}
//完整TCP服务端+客户端
//服务端
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
int main(){
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd==-1){
        perror("socket failed");
        return 1;
    }
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(listen_fd,(struct sockaddr*)&addr,sizeof(addr))==-1){
        perror("bind failed");
        close(listen_fd);
        return 1;
    }
    if(listen(listen_fd,5)==-1){
        perror("listen failed");
        close(listen_fd);
        return 1;
    }
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int conn_fd =
        accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
        if(conn_fd==-1){
            perror("accept");
            close(listen_fd);
            return 1;
        }
        char buf[1024] = {0};
        ssize_t n = read(conn_fd, buf, sizeof(buf) - 1);
        if(n>0){
            printf("received:%s\n", buf);
        }
        const char* resp = "hello";
        write(conn_fd, resp, strlen(resp));
        close(conn_fd);
        close(listen_fd);//必须先关闭通信socket再关闭监听socket
        return 0;
}
//客户端
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
int main(){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd==-1){
        perror("socket failed");
        return 1;
    }
    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);//将字符串网址转为网络地址
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) ==
        -1) {
        perror("connect");
        close(sockfd);
        return 1;
    }
    const char* msg = "Hello from client!";
    write(sockfd, msg, strlen(msg));
    char buf[1024] = {0};
    ssize_t n = read(sockfd, buf, sizeof(buf) - 1);
    if (n > 0)
        printf("Server response: %s\n", buf);
    close(sockfd);
    return 0;
}
//UDP服务端+客户端
//服务端
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(sockfd);
        return 1;
    }
    printf("UDP Server listening on port 8080...\n");

    char buf[1024] = {0};
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    ssize_t n = recvfrom(sockfd, buf, sizeof(buf) - 1, 0,
                         (struct sockaddr*)&client_addr, &client_len);
    if (n > 0)
        printf("Received: %s\n", buf);

    const char* resp = "Hello UDP client!";
    sendto(sockfd, resp, strlen(resp), 0, (struct sockaddr*)&client_addr,
           client_len);

    close(sockfd);
    return 0;
}
//客户端
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    const char* msg = "Hello UDP client!";
    sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr*)&server_addr,
           sizeof(server_addr));

    char buf[1024] = {0};
    struct sockaddr_in src_addr;
    socklen_t src_len = sizeof(src_addr);
    ssize_t n = recvfrom(sockfd, buf, sizeof(buf) - 1, 0,
                         (struct sockaddr*)&src_addr, &src_len);
    if (n > 0)
        printf("Server response: %s\n", buf);

    close(sockfd);
    return 0;
}
