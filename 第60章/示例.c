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
#define SERVICE "7"
#define BUF_SIZE 4096
int main(int argc, char* argv[]) {
    int sfd;
    ssize_t numread;
    char buf[BUF_SIZE];
    sfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in sockaddr;
    memset(&sockaddr, 0, sizeof(sockaddr));
    
}
//并发型TCP echo服务
//TCP echo客户端程序
