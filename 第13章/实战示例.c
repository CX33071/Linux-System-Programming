//验证内核缓冲的异步刷盘
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
int main(){
    int fd = open("test.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd==-1){
        perror("open failed");
        return 1;
    }
    char data[] = "内核缓冲异步刷盘测试数据";
    write(fd, data, strlen(data));
    struct stat st;
    fstat(fd, &st);
    printf("写入后文件名义大小:%ld字节\n", st.st_size);
    printf("写入后实际磁盘占用:");
    system("du -h test.txt");
    close(fd);
    printf("关闭文件后的实际磁盘占用:");
    system("du -h test.txt");
    unlink("test.txt");
    return 0;
}
//验证stdio行缓冲和全缓冲
#include <stdio.h>
#include <unistd.h>
int main(){
    printf("1");
    printf("1\n");
    FILE* fp = fopen("test.txt", "w");
    if(fp==NULL){
        perror("fopen failed");
        return 1;
    }
    for (int i = 0; i < 1000;i++){
        fputs("1", fp);
        if(i==500){
            printf("查看文件写入500行后是否有内容");
            sleep(2);
        }
    }
    fclose(fp);
    printf("1");
    fflush(stdout);
    unlink("test.txt");
    return 0;
}
//使用fsync保证数据持久化
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
int main(){
    int fd = open("test.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd==-1){
        perror("open failed");
        return 1;
    }
    char data[] = "1";
    write(fd, data, strlen(data));
    if(fsync(fd)==-1){
        perror("fsync failed");
        close(fd);
        return 1;
    }
    close(fd);
    unlink("test.txt");
    return 0;
}
//向内核提供顺序读取的建议
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
int main(){
    int fd = open("test.txt", O_RDONLY);
    if(fd==-1){
        perror("open failed");
        return 1;
    }
    if(posix_fadvise(fd,0,0,__POSIX_FADV_DONTNEED)==-1){
        perror("posix_fadvise failed");
        close(fd);
        return 1;
    }
    char buf[4096];
    ssize_t n;
    while((n=read(fd,buf,sizeof(buf)))>0){

    }
    close(fd);
    return 0;
}
//直接I/O读取文件
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
int main(){
    char* buf = aligned_alloc(4096, 4096);
    if(buf==NULL){
        perror("aligned_alloc failed");
        return 1;
    }
    int fd = open("test.txt", O_RDONLY | __O_DIRECT);
    if (fd == -1) {
        perror("open failed");
        free(buf);
        return 1;
    }
    if(read(fd,buf,4096)==-1){
        perror("read failed");
        close(fd);
        free(buf);
        return 1;
    }
    close(fd);
    free(buf);
    return 0;
}
//混合使用库函数和系统调用
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
int main(){
    FILE* fp = fopen("test.txt", "w+");
    if(fp == NULL) {
        perror("fopen failed");
        return 1;
    }
    int fd = fileno(fp);
    fputs("C库写入数据\n", fp);
    fflush(fp);
    char buf[100];
    lseek(fd, 0, SEEK_SET);
    read(fd, buf, sizeof(buf));
    printf("系统调用读取：%s", buf);
    write(fd, "系统调用写入数据\n", strlen("系统调用写入数据\n"));
    fseek(fp, 0, SEEK_SET);
    fgets(buf, sizeof(buf), fp);
    printf("C库读取：%s", buf);
    fclose(fp);
    unlink("test.txt");
    return 0;
}