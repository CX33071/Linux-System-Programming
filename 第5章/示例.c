//获取文件大小
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
int main(){
    int fd = open("test", O_RDONLY);
    if(fd==-1){
        perror("open failed");
        return 1;
    }
    off_t n = lseek(fd, 0, SEEK_END);
    if(n==-1){
        perror("lseek failed");
        close(fd);
        return 1;
    }
    close(fd);
}
//创建空洞文件，文件显示大小很大，但是实际占用磁盘空间很小
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
int main(){
    int fd = open("test", O_RDONLY | O_CREAT | O_TRUNC, 0644);
    if(fd==-1){
        perror("open failed");
        return 1;
    }
    int n1 = write(fd, "hello", 5);
    if(n1==-1){
        perror("write failed");
        close(fd);
        return 1;
    }
    off_t n2 = lseek(fd, 10000000, SEEK_CUR);
    if(n2==-1){
        perror("lseek failed");
        close(fd);
        return 1;
    }
    int n3 = write(fd, "nihao", 5);
    if(n3==-1){
        perror("write failed");
        close(fd);
        return 1;
    }
    off_t n4 = lseek(fd, 0, SEEK_END);
    colse(fd);
    return 0;
}
//获取/设置文件状态标志
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
int main(){
    int fd = open("test", O_RDWR);
    if(fd==-1){
        perror("open");
        return 1;
    }
    int flags = fcntl(fd, F_GETFL);
    if(flags==-1){
        perror("fcntl");
        close(fd);
        return 1;
    }
    if(flags&O_APPEND){
        printf("当前是追加模式\n");
    }else{
        printf("当前不是追加模式\n");
    }
    flags |= O_APPEND;
    if(fcntl(fd,F_SETFL,flags)==-1){
        perror("fcntl");
        close(fd);
        return 1;
    }
    close(fd);
    return 0;
}
//writev一次性写多个缓冲区
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/uio.h>
int main(){
    int fd = open("test", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd==-1){
        perror("open");
        return 1;
    }
    char head[] = "头部";
    char mid[] = "中间";
    char last[] = "尾部";
    struct iovec iov[3] = {{.iov_base = head, .iov_len = strlen(head)},
                           {.iov_base = mid, .iov_len = strlen(mid)},
                           {.iov_base = last, .iov_len = strlen(last)}};
    ssize_t n = writev(fd, iov, 3);
    if(n==-1){
        perror("writev");
        closse(fd);
        return 1;
    }
    close(fd);
    return 0;
}