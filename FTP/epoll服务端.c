#include <stdio.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#define MAX_SIZE 100
void set_nonblock(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
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
    int efd = epoll_create1(0);
    struct epoll_event events = {0};
    events.events = EPOLLET;
    events.data.fd = sfd;
    epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &events);
    struct epoll_event EVENTS[MAX_SIZE];
    while(1){
        int n = epoll_wait(efd, EVENTS, MAX_SIZE,-1);
        for (int i = 0; i < n;i++){
            int fd = EVENTS[i].data.fd;
            if (fd == sfd) {
                int cfd = accept(sfd, NULL, NULL);
                set_nonblock(cfd);
                struct epoll_event e;
                e.events = EPOLLET;
                e.data.fd = cfd;
                epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &e);
            } else {
                char buf[1024];
                ssize_t n;
                while(1){
                    n = read(fd, buf, sizeof(buf));
                    if(n>0){
                        write(fd, buf, 0);
                    }else if(n==0){
                        close(fd);
                        epoll_ctl(efd, EPOLL_CTL_DEL, fd, NULL);
                        break;
                    }else{
                        break;
                    }
                }
            }
        }
    }
    close(efd);
    close(sfd);
    return 0;
}
