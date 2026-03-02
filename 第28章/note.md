1.进程记账
内核会记录每个进程终止时的关键信息(如执行时间、CPU使用率，退出状态PID等)，并写入指定的记账文件/var/account/pacct
用途：监控监控、计费、按进程占用CPU时长计费
记账仅记录终止的进程，未终止的进程不会出现在记账文件中
需要root权限启用/配置进程记账，普通用户只能读取记账文件
记账不是默认开启的，需通过accton命令手动启用accton /var/account/pacct,关闭执行accton off
进程是二进制格式，要用lastcomm命令解析
进程记账会带来轻微系统开销
sudo accton /var/account/pacct
2.系统调用clone()
创建一个轻量级进程或线程，并精细控制父子进程共享的资源，地址空间、文件描述符表
#include <sched.h>
int clone(int(*fn)(void*),void*child_stack,int flags,void*arg,...);
