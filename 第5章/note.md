1.lseek
文件偏移计算，控制
lseek(int fd,off_t offset,int whence);
成功返回新的文件偏移量，失败返回-1
参数offset:偏移量，类型off_t,可正可负，正数向后移，负数向前移
lseek(fd,-10,SEEKEND)移到末尾前10字节
参数whence:SEEK_SET 从文件开头 offset>=0
          SEEK_CUR  从当前偏移  
          SEEK_END  从文件末尾
空洞文件应用场景：视频文件大文件预分配空间、数据库文件预留空间，减少碎片
2.fcntl
文件控制，获取/设置文件状态标志
flags&O_APPEND判断是否为这个标志
flags|=O_APPEND保留原有flags，添加O_APPEND
fcntl(fd,F_GETFL)获取当前flags
设置flags:先|添加标志，然后fcntl(fd,F_SETFL,flags)
不是所有flags都能设置，要改只能重新open
可以改的flags:O_APPEND  O_NONBLOCK  O_ASYNC
3.readv/writev分散/聚集IO
需要一次性续写多个不连续的缓冲区，比如先写头部，再写尾部，再写中间
函数原型：
        ssize_t readv(int fd,const struct iovec *iov,int iovcnt);
        ssize_t writev(int fd,const struct iovec*iov,int iovcnt);
        struct iovec{
            void*iov_base;缓冲区地址
            size_t iov_len;缓冲区长度
        }(#include <sys/uio.h>)
        参数3：要写几个缓冲区
writev(fd,iov,3)一次writev只切换一次内核态
