//正确的文件复制工具，实现cp命令的核心功能
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
int main(int argc,char*argv[]){
    if(argc!=3){
        fprintf(stderr, "用法：%s 源文件 目标文件", argv[0]);
        return 1;
    }
    int fd1 = open(argv[1], O_RDONLY);
    if(fd1==-1){
        perror("open failed");
        return 1;
    }
    int fd2 = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd2==-1){
        perror("open failed");
        return 1;
    }
    char buf[1024];
    ssize_t readlen, writelen;
    while((readlen=read(fd1,buf,sizeof(buf)))>0){
        char* p = buf;
        while(readlen>0){
            writelen = write(fd2, p, readlen);
            if(writelen==-1){
                perror("write failed");
                close(fd1);
                close(fd2);
                return 1;
            }
            readlen -= writelen;
            p += writelen;
        }
    }
    if(readlen==-1){
        perror("read");
        close(fd1);
        close(fd2);
        return 1;
    }
    close(fd1);
    close(fd2);
    return 0;
}
//用dup2实现重定向
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
int main(){
    int fd = open("test", O_RDONLY);
    if(fd==-1){
        perror("open failed");
        return -1;
    }
    if(dup2(fd,1)==-1){
        perror("dup2 failed");
        close(fd);
        return 1;
    }
    close(fd);
    return 0;
}
//原子操作，多个进程写文件
// int fd = open("test",O_RDONLY | O_CREAT | O_APPEND, 0644);