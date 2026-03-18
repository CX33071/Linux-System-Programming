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
#define SOCK_PATH "/tmp/us_xfr"
#define BUF_SIZE 100
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
}