//修改文件时间戳
#include <stdio.h>
#include <utime.h>
#include <sys/time.h>
int main(){
    //方式一:utime()秒级精度
    struct utimbuf times = {.actime = 1700000000, .modtime = 17000000000};
    if(utime("test.txt",&times)==-1){
        perror("utime failed");
        return 1;
    }
    //方式二:utimes()微秒级精度
    struct timeval tv[2];
    tv[0].tv_sec = 1700000000;
    tv[0].tv_usec = 500000;
    tv[1].tv_sec = 17000000;
    tv[1].tv_usec = 500000;
    if(utimes("test.txt",tv)==-1){
        perror("utimes failed");
        return 1;
    }
    return 0;
}
//纳秒级修改时间戳
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <utime.h>
#include <unistd.h>
int main(){
    // struct timespec ts[2];
    // ts[0].tv_sec = 0;
    // ts[0].tv_nsec = UTIME_NOW;
    // ts[1].tv_sec = 0;
    // ts[1].tv_nsec = UTIME_OMIT;
    // if(utimensat(AT_FDCWD,"test.txt",ts,0)==-1){
    //     perror("utimensat failed");
    //     return 1;
    // }
    // return 0;
}
//修改文件属主
#include <stdio.h>
#include <unistd.h>
int main(){
    if(chown("teat.txt",0,0)==-1){
        perror("chown failed");  // 没有root权限
        return 1;
    }
    return 0;
}
//检查文件权限
#include <stdio.h>
#include <unistd.h>
int main(){
    if(access("test.txt",F_OK)==-1){
        perror("文件不存在");
        return 1;
    }
    if(access("test.txt",R_OK|W_OK)==0){
        printf("拥有读写权限");
    }
    return 0;
}
//设置SUID位
// sudo chown root:root test
// sudo chmod u+s test
//修改umask
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
int main(){
    mode_t old_umask = umask(0002);
    int fd = open("umask.txt", O_WRONLY, O_CREAT, 0644);
    close(fd);
    system("ls -l umask.txt");
    umask(old_umask);
    return 0;
}
//修改文件权限
#include <stdio.h>
#include <sys/stat.h>
int main(){
    if(chmod("test.txt",0644)==-1){
        perror("chmod failed");
        return 1;
    }
    return 0;
}