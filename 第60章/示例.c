//迭代型UDP echo服务器
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
    printf("Iterative UDP echo server listening on port %d...\n", PORT);

    char buf[BUF_SIZE];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    // 迭代处理：一次处理一个客户端
    while (1) {
        ssize_t n = recvfrom(sockfd, buf, BUF_SIZE, 0,
                             (struct sockaddr*)&client_addr, &client_len);
        if (n > 0) {
            sendto(sockfd, buf, n, 0, (struct sockaddr*)&client_addr,
                   client_len);
        }
    }
    close(sockfd);
    return 0;
}

#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#define SERVICE "7"
#define BUF_SIZE 4096
#define IS_ADDR_STR_LEN 46
int main(){
    int sfd;
    char buf[BUF_SIZE];
    ssize_t numread,numsent;
    socklen_t addrlen;
    sfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_storage claddr;
    struct sockaddr_in svaddr;
    memset(&svaddr, 0, sizeof(svaddr));
    svaddr.sin_family = AF_INET;
    svaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    svaddr.sin_port = htons(atoi(SERVICE));
    bind(sfd, (struct sockaddr*)&svaddr, sizeof(svaddr));
    for (;;){
        addrlen = sizeof(claddr);
        numread = recvfrom(sfd, buf, BUF_SIZE, 0, (struct sockaddr*)&claddr,
                           &addrlen);
        if(numsent=(sendto(sfd,buf,numread,0,(struct sockaddr*)&claddr,addrlen))!=numread){
            syslog(LOG_WARNING, "sent %zd bytes,expect %zd bytes\n", numsent,
                   numread);
            close(sfd);
            exit(1);
        }
    }
}
//UDP echo客户端程序
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <syslog.h>
#include <unistd.h>
#include <string.h>
#define SERVICE "7"
#define BUF_SIZE 4096
int main(int argc, char* argv[]) {
    int sfd;
    ssize_t numread;
    char buf[BUF_SIZE];
    sfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = htonl(argv[1]);
    sockaddr.sin_port = htons(atoi(SERVICE));
    connect(sfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr));
    for (int j = 2; j < argc;j++){
        int len = strlen(argv[j]);
        if(write(sfd,argv[j],len)!=len){
            perror("write");
            close(sfd);
            return 1;
        }
        numread = read(sfd, buf, BUF_SIZE);
        if(numread==-1){
            perror("read");
            close(sfd);
            return 1;
        }
        return 0;
    }
}
//并发型TCP echo服务
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <syslog.h>
#include <signal.h>
#include<sys/wait.h>
#define SERVICE "7"
#define BACKLOG 10
#define BUF_SIZE 4096
void handlesig(void* sig){
    while(waitpid(-1,NULL,WNOHANG)>0){
        continue;
    }
}
void handleRequest(int sfd){
    char buf[BUF_SIZE];
    ssize_t numread;
    ssize_t numsent;
    while (numread = read(sfd, buf, sizeof(BUF_SIZE)) > 0) {
        if(numsent=(write(sfd,buf,numread))!=numread){
            syslog(LOG_WARNING, "sent %zd bytes,expect %zd bytes\n", numsent,
                   numread);
            close(sfd);
            return 1;
        } 
    }
}
int main(){
    signal(SIGCHLD, handlesig);
    int sfd, cfd;
    struct sockaddr_storage addr;//兼容IPV4和IPV6地址
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    listen(sfd, BACKLOG);
    for (;;){
        cfd = accept(sfd, NULL, NULL);
        switch (fork())
        {
        case -1:
            perror("fork");
            close(cfd);
            break;
        case 0:
            close(sfd);
            handleRequest(cfd);
            break;
        default:
            close(cfd);
            break;
        }
    }
}
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT 8080
#define BUF_SIZE 1024

// 处理僵尸进程：SIGCHLD 信号处理函数
void sigchld_handler(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

int main() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        return 1;
    }

    // 允许端口复用
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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
    printf("Concurrent TCP echo server listening on port %d...\n", PORT);

    // 注册 SIGCHLD 信号处理，避免僵尸进程
    signal(SIGCHLD, sigchld_handler);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int conn_fd =
            accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
        if (conn_fd == -1) {
            perror("accept");
            continue;
        }

        // fork 子进程处理连接
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            close(conn_fd);
            continue;
        }

        if (pid == 0) {        // 子进程
            close(listen_fd);  // 子进程不需要监听 socket
            char buf[BUF_SIZE];
            ssize_t n;
            while ((n = read(conn_fd, buf, BUF_SIZE)) > 0) {
                write(conn_fd, buf, n);  // echo 回显
            }
            close(conn_fd);
            _exit(0);  // 子进程退出
        } else {                  // 父进程
            close(conn_fd);       // 父进程不需要连接 socket
        }
    }
    close(listen_fd);
    return 0;
}

//TCP echo服务器多线程版
//TCP echo服务器I/O多路复用版
