1.进程就是程序的一次执行实例，内核为每个进程分配：
    独立的地址空间
    进程ID（PID）
    文件描述符表
    环境变量
    优先级、状态等属性
2.进程相关API
    getpid()获取当前进程PID
    getppid()获取父进程PID
    getpgid(0)获取进程组ID，0表示当前进程
    用类型pid_t接收
PID从1开始分配，最大值默认是32768,init/systemd是PID=1，用完循环
子进程的PPID是父进程的PID
父进程退出后，子进程的PPID变为1,被init/systemd收养，成为孤儿进程
3.进程内存布局
高->低：栈  局部变量、函数调用栈帧（向下生长）
       堆   动态内存（向上生长）
       BSS段    未初始化全局/静态变量（内核清0）、不占磁盘空间
       数据段data   已初始化全局/静态变量、占磁盘空间
       代码段   可执行指令（只读）
       内核预留区
函数地址=代码段
4.环境变量
操作系统的通用信息卡，全局可见，可修改环境变量但是仅自己生效，管理员修改环境变量所有新启动的进程都用新的环境变量
环境：当前终端/进程运行的地方
    eg:开发环境：ENV=dev，程序连接测试数据库；
    生产环境：ENV=prod，程序连接正式数据库；
    仅改一个环境变量，程序行为就完全不同，不用改一行代码
本质：键=值的字符串数组，是操作系统为进程提供的配置信息，全局可访问，
用途：控制程序行为（PATH指定命令查找路径）、配置系统环境（HOME指定用户著目录）、传递参数
PATH    可执行程序的查找路径，多个路径用：分隔，类似地址簿，没有PATH，执行命令时必须写全路径/bin/ls
HOME    当前用户的著目录
PWD 当前工作目录
终端：
    - 查看环境变量
        env(所有)
        echo $PATH(指定)
    - 临时设置
        export MY_VAR="hello"
    - 删除
        unset MY_VAR
C:
    提供两种核心方式操作环境变量：getenv(),setenv(),unsetenv()、直接访问environ全局变量
    必备头文件：#include <stdlib.h>//getenv,settenv
              #include <unistd.h>//environ
    getenv(const char*name)获取环境变量，返回指向VALUE的指针，只读不可修改，获取不存在的变量返回NULL
    setenv(const char*name,const char*value,int overwrite)设置环境变量，overwrite=1覆盖已有变量，overwrite=0不覆盖，成功返回0
    unsetenv(const char*name)删除环境变量，成功返回0
    environ是一个全局字符串数组，存储所有环境变量，以NULL结尾：
                先声明environ全局变量，然后遍历所有环境变量
5.通过main函数第3个参数也能操作环境变量，main函数的第3个参数是环境变量数组，char *envp[]
6.注意：
    子进程集成父进程的环境变量，但是子进程修改不影响父进程
    getenv()返回的字符串不可直接修改，可通过strdup()复制，再修改副本，或setenv()
    环境变量可以有空值，getenv()返回空字符串，不是NULL
    execvp执行命令时，就是从PATH环境变量中查找命令的路径
    

