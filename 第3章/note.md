1.系统调用的执行流程
open不是直接调用内核，是通过libc提供的open包装函数：把参数放到寄存器——>触发软中断——>进入内核——>获取返回值——>设置errno
系统调用开销大主要是因为上下文保存/恢复
2.头文件
系统调用    <unistd.h>  read/write/close
文件控制    <fcntl.h>   open/fcntl
错误处理    <errno.h>   errno/strerror
进程管理    <sys/types.h><unistd.h>    pid_t/getpid
内存分配    <stdlib.h>  malloc/free
字符串操作  <string.h>  strlen/strcpy
man 2 open查需要的头文件，man 2 系统调用名 查系统调用，man 3 库函数名 查库函数
3.编译
gcc -E test.c -o test.i预处理（展开头文件、宏）
gcc -S test.i -o test.s编译（生成汇编）
gcc -c test.s -o test.o汇编（生成机器码）
gcc test.o -o test链接（生成可执行文件）
常用编译选项
-Wall   显示所有警告
-o      指定输出文件名    
-g      生成调试信息
-O2     优化代码
