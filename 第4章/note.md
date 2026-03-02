1.open
int open(const char*pathname,int flags,mode_t mode)
参数1：可以是相对路径或者绝对路径
参数2：必选：O_RDONLY   只读
           O_WRONLY   只写
           O_RDWR     读写
      可选：O_CREAT    文件不存在则创建
           O_TRUNC    文件存在则清空内容
           O_APPEND   每次追加到文件末尾(原子操作)
           O_EXCL     与O_CREAT一起用，文件已存在则open失败，避免覆盖
           O_NONBLOCK   非阻塞模式
参数3：仅O_CREAT需要
        权限值:用8进制表示（0开头）
        0->8进制
        所有者(u)
        组(g)
        其他(o)
        实际权限=mode&~umask(umask默认0022)
2.read/write
    ssize_t是有符号整数，size_t是无符号整数
    read的参数：读取的数据存在buf缓冲区里，期望读取的最大字节数
    write的参数：要写入的数据开始存放在buf这个缓冲区，期望写入的字节数
    read返回值：
             >0 实际读取的字节数
             =0 到达文件末尾EOF
    write返回值：
             >0 实际写入的字节数 
read和write可能部分读写，必须循环处理
3.dup/dup2
dup(oldfd)复制oldfd,返回最小的未使用fd
dup2(oldfd,newfd)强制把newfd指向oldfd的文件表项，如果newfd已打开，先关闭
4.原子操作——多进程安全
两个进程同时写一个文件，都lseek到文件末尾，可能会覆盖
方法：直接用O_APPEND打开文件，保证每次write前内核自动把文件偏移移到末尾且这个过程是原子的，不可断
多进程写同一个文件必须加O_APPEND

