//写一个POSIX程序
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
int main(){
    int fd = open("test.", O_RDONLY);
    if(fd==-1){
        perror("open failed");
        return 1;
    }
    char* buf = "123";
    size_t n = read(fd, buf, strlen(buf));
    if(n==-1){
        perror("read failed");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}