//完整读函数，按指定长度读完数据
#include <unistd.h>
ssize_t read_full(int fd,void*buf,size_t count){//fd要读取的文件描述符，buf存储读取数据的缓冲区，count期望读取的字节总数
    size_t total = 0;
    ssize_t n;
    while(total<count){
        n = read(fd, (char*)buf + total, count - total);//buf从total位置开始写，count-total本次最多读
        if(n==-1){
            return -1;
        }
        if(n==0){
            break;//对端关闭连接，read返回0
        }
        total += n;
    }
    return total;
}
//完整写函数，write返回值只有-1和>0
ssize_t write_full(int fd,const void*buf,size_t count){
    size_t total = 0;
    ssize_t n;
    while(total<count){
        n = write(fd, (const char*)buf + total, count - total);
        if(n==-1){
            return -1;
        }
        total += n;
    }
    return total;
}
// shutdown()示例
//  客户端发送完数据后半关闭
// const char* msg = "Hello, server!";
// write_full(sockfd, msg, strlen(msg));
// shutdown(sockfd, SHUT_WR);  // 告知服务端发送结束
// // 继续读取服务端响应
// char buf[1024];
// ssize_t n = read_full(sockfd, buf, sizeof(buf) - 1);
// if (n > 0) {
//     buf[n] = '\0';
//     printf("Server response: %s\n", buf);
// }
// close(sockfd);

//偷看数据
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
int main(){
    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    char peek_buf[32];
    ssize_t n = recv(sockfd, peek_buf, sizeof(peek_buf), MSG_PEEK);
    if (n > 0) {
        printf("Peeked data: %.*s\n", (int)n, peek_buf);
    }
// 再次 read() 仍能读到刚才的数据
char read_buf[32];
n = read(sockfd, read_buf, sizeof(read_buf));
}
//高效发送文件
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
int send_file(int sockfd, const char* filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1)
        return -1;
    off_t offset = 0;
    struct stat st;
    fstat(fd, &st);
    ssize_t sent = sendfile(sockfd, fd, &offset, st.st_size);
    close(fd);
    return sent;
}
//获取套接字地址
#include <arpa/inet.h>
void print_local_addr(int sockfd) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    getsockname(sockfd, (struct sockaddr*)&addr, &len);

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    printf("Local addr: %s:%d\n", ip, ntohs(addr.sin_port));
}
void print_peer_addr(int sockfd) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    getpeername(sockfd, (struct sockaddr*)&addr, &len);

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    printf("Peer addr: %s:%d\n", ip, ntohs(addr.sin_port));
}
