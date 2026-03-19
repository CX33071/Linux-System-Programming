//迭代型UDP echo服务器
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
    struct sockaddr_storage addr;
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
//TCP echo客户端程序
