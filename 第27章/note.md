1.execve()
执行新程序，用一个新程序完全替换当前进程的地址空间，执行新程序，进程PID不变，相当于变身
#include <unistd.h>
int execve(const char*filename,char*const argv[],char*const envp[]);
filename:要执行的程序文件路径，可相对
argv:以NULL结尾，argv[0]程序名
envp:环境变量，键值对形式，以NULL结尾
成功没有返回值，失败返回-1并设置errno
新进程继承的资源：PID、PPID、打开的文件描述符、信号掩码、当前工作目录
execve执行成功，之后的代码不会执行
2.exec库函数
exec家族所有函数最终都会调用底层的execve()系统调用，不同后缀只是为了简化参数传递和路径查找，本质是——语法糖
p   PATH    不用写程序的完整路径，内核去PATH环境变量里找
l   LIST    参数用逗号分隔的列表传递execl("/bin/ls","ls","-l",NULL)，必须以NULL结尾
v   vector  参数用字符串数组传递char *argv[]={"ls","-l",NULL}
e   environment 可以自定义环境变量数组，默认继承父进程环境变量(通过全局变量environ)，加e可以替换
后缀可以组合
环境变量PATH：是一个由冒号分隔的目录列表，p后缀会按顺序在这些目录中查找可执行文件
fexecve()执行由文件描述符指代的程序，而非文件名，执行该文件描述符执指向的程序，优势：避免了路径查找
int fexecve(int fd,char*const argv[],char*const envp[])
3.解释器脚本
解时期脚本不是可执行的机器码，需要依赖解释器程序解析执行，开头第一行的#！(shebang)告诉内核：请调用指定的解释器程序来执行这个脚本的内容，内核会通过execve替换程序为解释器程序，并把脚本路径作为参数传给解释器
#！后根的就是解释器的完整路径#！/bin/bash
4.文件描述符与exec
默认情况下exec会继承所有打开的文件描述符，可通过fcntl()设置FD_CLOEXEC标志使文件描述符在exec时自动关闭，避免泄漏给新程序
    fcntl(fd,F_SETFD,FD_CLOEXEC)
5.信号与exec
信号处理函数在exec后会被重置为默认行为，因为新程序的地址空间中不存在原处理函数
但是被阻塞的信号掩码和被忽略的信号会被继承
6.执行shell命令:system()
system()是一个库函数，用于在C程序中执行shell命令，内部实现为fork()+exec()+wait()并返回子进程状态，期间会临时忽略SIGINT和SIGQUIT信号，避免中断system()调用
#include <stdlib.h>
int system(const char*command);
返回shell的退出状态，可用WEXITSTATUS（）解析，如果command为NULL，返回非0表示shell可用，返回0表示shell不可用
