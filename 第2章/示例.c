//对比系统调用和库函数
//系统调用
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
int main(){
    int fd = open("test", O_WRONLY | O_CREAT, 0644);
    if(open==-1){
        perror("open failed");
        return -1;
    }
    char* buf = "系统调用写的内容\n";
    ssize_t n = write(fd, buf, strlen(buf));
    if(n==-1){
        perror("write failed");
        close(fd);
        return 1;
    }
    close(fd);
    return 0;
}
//库函数
#include <stdlib.h>
#include <stdio.h>
int main(){
    FILE* fp = fopen("test", "w");
    if(fp==NULL){
        perror("fopen failed");
        return 1;
    }
    fprintf(fp, "库函数写的内容\n");
    fclose(fp);
    return 0;
}
//错误处理模板
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
int main(){
    int fd = open("test", O_RDONLY);
    if(fd==-1){
        perror("open failed");
        printf("desc:%s\n" ,strerror(errno));
        return 1;
    }
    close(fd);
    return 0;
}
//重定向标准输出
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
int main(){
    int fd = open("test", O_RDONLY);
    if(fd==-1){
        perror("open failed");
        return 1;
    }
    close(1);
    int new_fd = dup(fd);
    if(new_fd!=1){
        perror("dup failed");
        close(fd);
        return 1;
    }
    close(fd);
    printf("1");//写到文件
    fprintf(stderr, "1");//写到屏幕
    return 0;
}
