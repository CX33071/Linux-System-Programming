//select监控标准输入和socket
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/select.h>
#include <netinet/in.h>
int main(){
    struct sockaddr_in addr = {0};
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(sfd, (struct sockaddr*)&addr, sizoef(addr));
    listen(sfd, 5);
    fd_set readfds;
    int max_fd = sfd > STDIN_FILENO ? sfd : STDIN_FILENO;
    while(1){
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sfd, &readfds);
        int ret = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if(FD_ISSET(STDIN_FILENO,&readfds)){
            char buf[32];
            read(STDIN_FILENO, buf, sizeof(buf));
            printf("you typed:%s", buf);
        }
        if(FD_ISSET(sfd,&readfds)){
            int cfd = accept(sfd, NULL, NULL);
            printf("nem client connected :%d\n", cfd);
            close(cfd);
        }
    }
    close(sfd);
    return 0;
}
//poll监控多个客户端
// poll 监听的 fd 会越来越多（初始只有 listen_fd，后续加客户端 fd），但 POLLIN事件对应两种完全不同的行为：
// 当 listen_fd 触发 POLLIN：表示「有新客户端发起连接」，需要调用accept 处理； 当客户端 fd 触发POLLIN：表示「该客户端发来了数据」，需要调用 read /write 处理
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#define MAX_CLIENTS 10
    int main() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(listen_fd, 5);
    struct pollfd fds[MAX_CLIENTS];
    int nfds = 1;
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    while (1) {
        int ret = poll(fds, nfds, -1);  // -1 表示永久阻塞
        if (ret == -1) {
            perror("poll");
            break;
        }
        for (int i = 0; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                if (fds[i].fd == listen_fd) {
                    // 新连接
                    int conn = accept(listen_fd, NULL, NULL);
                    fds[nfds].fd = conn;
                    fds[nfds].events = POLLIN;
                    nfds++;
                    printf("New client: %d\n", conn);
                } else {
                    // 客户端数据
                    char buf[1024];
                    ssize_t n = read(fds[i].fd, buf, sizeof(buf));
                    if (n <= 0) {
                        close(fds[i].fd);
                        // 从数组中移除
                        fds[i] = fds[nfds - 1];
                        nfds--;
                        i--;
                    } else {
                        write(fds[i].fd, buf, n);  // echo
                    }
                }
            }
        }
    }
    close(listen_fd);
    return 0;
}
//epoll高并发TCP服务器
#include <stdio.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#define MAX_SIZE 100
void set_nonblock(int fd){
    int flags=fcntl(fd,F_GETFL,0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
int main(){
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    set_nonblock(sfd);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(sfd, (struct sockaddr*)&addr, sizeof(addr));
    listen(sfd, 100);
    int epfd=epoll_create(0);
    struct epoll_event events = {0};
    events.events = EPOLLET;
    events.data.fd = sfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &events);
    struct epoll_event EVENTS[MAX_SIZE];
    while (1) {
        int n = epoll_wait(epfd, EVENTS, MAX_SIZE - 1,0);
        for (int i = 0; i < n;i++){
            if(EVENTS[i].data.fd==sfd){
                int cfd = accept(sfd, NULL, NULL);
                set_nonblock(cfd);
                events.events = EPOLLET | EPOLLIN;
                events.data.fd = cfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &events);
            }else{
                char buf[1024];
                while((n=read(EVENTS[i].data.fd,buf,sizeof(buf)))>0){
                    write(EVENTS[i].data.fd,buf,n);
                }
                if(n==0){
                    close(EVENTS[i].data.fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, sfd, NULL);
                }
            }
        }
    }
    close(epfd);
    close(sfd);
    return 0;
}
//pselect处理信号和fd就绪竞态条件
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/select.h>
int sig_all = 0;
void sig_handle(int sig) {
    sig_all = 1;
}
int main(){
    signal(SIGUSR1, sig_handle);
    sigset_t new_mask;
    sigemptyset(&new_mask);
    sigaddset(&new_mask, SIGUSR1);
    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(STDIN_FILENO, &readset);
    int ret = pselect(1, &readset, NULL, NULL, NULL, &new_mask);
    if(ret==-1){
        if(errno==EINTR){
            printf("被信号打断\n");
        }else{
            perror("pselect failed\n");
            return 1;
        }
    }else{
        char buf[1024];
        read(STDIN_FILENO,buf,sizeof(buf));
    }
    if(sig_all){
        printf("处理信号\n");
    }
    return 0;
}
//self-pipe技巧
//当触发ctrl+c时，往管道里写字让管道读端可读，进入if,break退出
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <signal.h>
int pipefd[2];
void sig_handle(int sig) {
    write(pipefd[1], "x", 1);
}
int main(){
    pipe(pipefd);
    signal(SIGINT, sig_handle);
    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(STDIN_FILENO, &readset);
    FD_SET(pipefd[0], &readset);
    while(1){
        select(1, &readset, NULL, NULL, NULL);
        if(FD_ISSET(pipefd[0],&readset)){
            char c;
            read(pipefd[0], c, 1);
        }
        if(FD_ISSET(STDIN_FILENO,&readset)){
            char buf[1024];
            read(STDIN_FILENO, buf, sizeof(buf));
        }
    }
    close(pipefd[0]);
    close(pipefd[1]);
    return 0;
}
//epoll高并发服务器
#include <stdio.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/epoll.h>
void set_nonblock(int fd){
    int flag=fcntl(fd,F_GETFL,0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}
int main(){
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    set_nonblock(sfd);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(sfd, (struct sockaddr*)&addr, sizeof(addr));
    listen(sfd, 10);
    int epfd=epoll_create(0);
    struct epoll_event EVENTS[10];
    struct epoll_event events = {0};
    events.events = EPOLLET;
    events.data.fd = sfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &events);
    while(1){
        int n=epoll_wait(epfd, EVENTS, 9, NULL);
        for (int i = 0; i < n;i++){
            if(EVENTS[i].events==EPOLLET){
                if(EVENTS[i].data.fd==sfd){
                    int cfd = accept(EVENTS[i].data.fd, NULL,NULL);
                    events.events = EPOLLET | EPOLLIN;
                    events.data.fd = cfd;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &events);
                }else{
                    int n;
                    char buf[1024];
                    while (n = read(EVENTS[i].data.fd, buf, sizeof(buf)) > 0) {
                        write(EVENTS[i].data.fd, buf, sizeof(buf));
                    }
                    if(n==0){
                        close(EVENTS[i].data.fd);
                        epoll_ctl(epfd, EPOLL_CTL_DEL, sfd,
                                  &events);
                    }
                }
            }
        }
    }
    close(epfd);
    close(sfd);
    return 0;
}
//线程并发型服务器
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <syslog.h>
typedef struct{
    int cfd;
} Threaddata;
void pthread_handle(void* arg) {
    Threaddata* data = (Threaddata*)arg;
    free(arg);
    int cfd = data->cfd;
    ssize_t numread;
    ssize_t numsent;
    char buf[1024];
    while(numread=read(cfd,buf,sizeof(buf))>0){
        if(numsent=(write(cfd,buf,sizeof(buf)))!=numread){
            syslog(LOG_WARNING, "sent %zd bytes,expect %zd bytes\n", numsent,
                   numread);
            close(cfd);
            exit(1);
        }
    }
    close(cfd);
    return NULL;
}
int main(){
    int sfd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(sfd, (struct sockaddr*)&addr, sizeof(addr));
    listen(sfd, 10);
    pthread_t t1;
    while (1) {
        int cfd = accept(sfd, NULL, NULL);
        Threaddata* data = (Threaddata*)malloc(sizeof(Threaddata));
        data->cfd = cfd;
        pthread_create(&t1, NULL, pthread_handle, data);
    }
    close(sfd);
    return 0;
}