//绑定UNIX domain socket
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
int main() {
    const char* sockname = "/tmp/mysock";
    int sfd;
    struct sockaddr_un addr;
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sfd==-1){
        perror("socket");
        return 1;
    }
    memset(&addr, 0, sizeof(struct sockaddr_un));//确保结构体中所有字段为0
    addr.sun_family = AF_INET;
    strncpy(addr.sun_path, sockname, sizeof(addr.sun_path) - 1);//减1来确保这个字段总有null结尾
    if(bind(sfd,(struct sockaddr*)&addr,sizeof(struct sockaddr_un))==-1){
        perror("bind");
        return 1;
    }
}
//创建一个抽象socket绑定
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
int main(){
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(&addr.sun_path[1], "xyz", sizeof(addr.sun_path) - 2);
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un));
    return 0;
}
//一个简单的UNIX domain 流socket服务器
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#define SOCK_PATH "/tmp/us_xfr"
#define BUF_SIZE 100
#define BACKLOG 5
int main(int argc,char*argv[]){
    struct sockaddr_un addr;
    int sfd, cfd;
    ssize_t numread;
    char buf[BUF_SIZE];
    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(remove(SOCK_PATH)==-1&&errno!=ENOENT){
        perror("remove");
        return 1;
    }
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path)-1);
    bind(sfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un));
    listen(sfd, BACKLOG);
    for (;;){
        cfd = accept(sfd, NULL, NULL);
        while(numread=read(cfd,buf,BUF_SIZE)>0){
            if(write(STDOUT_FILENO,buf,numread)!=numread){
                perror("write");

            }
        }
        if(numread==-1){
            perror("read");
            return 1;
        }
        close(cfd);
    }
}
//一个简单的UNIX domain 流socket客户端
#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#define SOCK_PATH "/tmp/us_xfr"
#define BUF_SIZE 100
#define BACKLOG 5
int main(){
    struct sockaddr_un addr;
    int sfd;
    ssize_t numread;
    char buf[BUF_SIZE];
    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path));
    connect(sfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un));
    while(numread=read(STDIN_FILENO,buf,BUF_SIZE)>0){
        if(write(sfd,buf,numread)!=numread){
            perror("write");
        }
    }
    if(numread==-1){
        perror("read");
        return 1;
    }
    exit(0);
}
//  ./服务器 > b &
//  cat *.c > a
//  ./客户端 < a        kill diff
//
//一个简单的UNIX domain 数据报服务器
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#define BUF_SIZE 10
#define SOCK_PATH "/tmp/ud_ucase"
int main(int argc,char*argv[]){
    struct sockaddr_un svaddr, claddr;
    int sfd, j;
    ssize_t numread;
    socklen_t len;
    char buf[BUF_SIZE];
    sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(remove(SOCK_PATH)==-1&&errno==ENOENT){
        perror("remove");
        return 1;
    }
    memset(&svaddr, 0, sizeof(struct sockaddr_un));
    svaddr.sun_family = AF_UNIX;
    strncpy(svaddr.sun_path, SOCK_PATH, sizeof(svaddr.sun_path) - 1);
    bind(sfd, (struct sockaddr*)&svaddr, sizeof(struct sockaddr_un));
    for (;;){
        len = sizeof(struct sockaddr_un);
        numread =
            recvfrom(sfd, buf, BUF_SIZE, 0, (struct sockaddr*)&claddr, &len);
        sendto(sfd, buf, numread, 0, (struct sockaddr*)&claddr, len);
    }
}
//一个简单的UNIX domain数据报客户端
#include <ctype.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#define BUF_SIZE 10
#define SOCK_PATH "/tmp/ud_ucase"
int main(int argc,char*argv[]){
    struct sockaddr_un svaddr, claddr;
    int sfd, j;
    size_t msglen;
    ssize_t numread;
    char buf[BUF_SIZE];
    sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
    memeset(&claddr, 0, sizeof(struct sockaddr_un));
    claddr.sun_family = AF_UNIX;
    snprintf(claddr.sun_path, sizeof(claddr.sun_path), "/tmp/ud_ucase_cl.%ld",
             (long)getpid());//为客户端绑定唯一的临时地址，流socket客户端调用connect时内核会自动绑定临时地址，无需手动bind，数据报需要手动bind客户端地址，因为如果不手动bind内核绑定的临时地址是匿名的不可控的，会导致服务器无法稳定回包，getpid确保地址唯一
    bind(sfd, (struct sockaddr*)&claddr, sizeof(struct sockaddr_un));//数据报客户端必须绑定地址，否则服务器无法回包
    memset(&svaddr, 0, sizeof(struct sockaddr_un));//初始化服务器地址
    svaddr.sun_family = AF_UNIX;
    strncpy(svaddr.sun_path, SOCK_PATH, sizeof(svaddr.sun_path) -1);
    for (j = 1; j < argc;j++){//遍历命令行参数，逐个发送给服务器
        msglen = strlen(argv[j]);
        if(sendto(sfd,argv[j],msglen,0,(struct sockaddr*)&svaddr,sizeof(struct sockaddr_un))!=msglen){
            perror("sendto");
            remove(claddr.sun_path);
            close(sfd);
            return 1;
        }
        numread = recvfrom(sfd, buf, BUF_SIZE, 0, NULL, NULL);//接收服务器的响应数据报
        remove(claddr.sun_path);
        close(sfd);
        return 0;
    }
}