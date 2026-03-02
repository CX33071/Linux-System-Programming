//execve()
#include <stdio.h>
#include <unistd.h>
int main(){
    char* argv[] = {"ls", "-l", "/", NULL};
    char* envp[] = {"PATH=/bin:usr/bin","USER=root",NULL};
    execve("/bin/ls", argv, envp);//若成功不会执行下面的代码
    perror("execve");
    return 1;
}
//fcntl避免泄漏文件描述符给新程序
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
int main(){
    int fd = open("test.txt",O_RDONLY);
    if(fd==-1){
        perror("open");
        return 1;
    }
    if(fcntl(fd,F_SETFD,FD_CLOEXEC)==-1){
        perror("fcntl");
        return 1;
    }
    execlp("ls", "ls", "-l", NULL);
    perror("execlp");
    return 0;
}
//system()
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(){
    int status = system("ls -l /");
    if(status==-1){
        perror("system failed");
        return 1;
    }
    if(WIFEXITED(status)==0){
        printf("exited status:%d", WEXITSTATUS(status));
    }
    return 0;
}