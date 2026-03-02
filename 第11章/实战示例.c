//查询编译和运行时限制
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#define PATH_MAX 4069
int main(){
    printf("PATH_MAX:%d\n", PATH_MAX);
    long open_max = sysconf(_SC_OPEN_MAX);
    if(open_max!=-1){
        printf("进程最大打开文件数：%ld\n", open_max);
    }
    long page_size = sysconf(_SC_PAGE_SIZE);
    if(page_size!=-1){
        printf("系统内页大小:%ld字节\n", page_size);
    }
    long name_max = pathconf(".", _PC_NAME_MAX);
    if(name_max!=-1){
        printf("当前目录文件名最大长度：%ld\n", name_max);
    }
    return 0;
}
//查询管道的原子写大小
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
int main(){
    int pipefd[2];
    if(pipe(pipefd)==-1){
        perror("pipe failed");
        return 1;
    }
    long pipe_buf = fpathconf(pipefd[1], _PC_PIPE_BUF);
    if(pipe_buf!=-1){
        printf("管道原子写最大字节数：%ld\n", pipe_buf);
    }
    close(pipefd[0]);
    close(pipefd[1]);
    return 0;
}
//处理不确定的路径长度限制
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
int main(){
    char* buf = NULL;
    size_t buf_size = PATH_MAX;
    buf = (char*)malloc(buf_size);
    if(buf==NULL){
        perror("malloc failed");
        return 1;
    }
    while(getcwd(buf,buf_size)==NULL){
        if(errno==ERANGE){
            buf_size *= 2;
            char* new_buf = realloc(buf, buf_size);
            if(new_buf==NULL){
                perror("realloc failed");
                return 1;
            }
            new_buf = buf;
        } else {
            perror("getcwd failed");
            free(buf);
            return 1;
        }
    }
    printf("当前工作目录:%s\n", buf);
    free(buf);
    return 0;
}
//检查系统是否支持线程和异步I/O
#include <stdio.h>
#include <unistd.h>
int main(){
    long has_threads = sysconf(_SC_THREADS);
    if(has_threads>0){
        printf("系统支持POSIX线程\n");
    }else{
        printf("系统不支持POSIX线程\n");
    }
    long has_io = sysconf(_SC_ASYNCHRONOUS_IO);
    if(has_io>0){
        printf("系统支持异步I/O\n");
    }else{
        printf("系统不支持异步I/O");
    }
    return 0;
}