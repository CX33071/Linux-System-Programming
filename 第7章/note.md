1.核心函数
malloc(size)    分配size字节，内容未初始化，失败返回NULL，分配的内存可能比请求的大（对齐）
calloc(n,size)  分配n*size字节，内容清0,比malloc慢，因为要清0,适合数组
realloc(ptr,size)   调整ptr的大小，扩容尽量原地扩容，不行则拷贝到新地址，size=0相当于free,返回新地址，旧地址失效，失败时旧地址仍有效
2.常见内存错误
    野指针：free后指针仍指向已释放的内存，操作会崩溃
      解决：free后把指针置为NULL
    double free:重复free
      解决：free后把指针置为NULL
    缓冲区溢出：strcpy(p,"hello")
      解决：用strncpy或snprintf限制长度
    内存泄漏：没有free
      解决：用valgrind检测
        gcc leak.c -o leak -g 
        valgrind --leak-check=full ./leak
valgrind会输出：
• 越界写的位置；
• 重复free的位置；
• 内存泄漏的大小
3.堆管理底层实现
先清楚堆的两层管理模型：程序调用malloc/free->用户态堆管理器(ptmalloc2/glibc)向操作系统申请大内存，工具：brk/mmap->操作系统内核，管理进程的虚拟地址空间->物理内存
内核层：只负责给堆管理器分配大块连续的虚拟内存，不关心小块内存分配
用户态层：堆管理器把大块内存切割成小内存块，分配给应用程序，回收后复用，减少系统调用
    操作系统如何给堆分配内存？
    进程启动时，内核给堆分配一个初始区域(很小)，用program break堆顶指针标记堆的末尾，堆管理器通过两个核心系统调用向内核申请/释放：
    - brk()/sbrk()
    扩展堆
    brk(addr)把堆顶指针移动到addr,若addr大于堆顶指针则扩展堆申请内存，若小于则释放内存收缩堆
    sbrk(1024)堆顶向后移1024字节，返回旧堆顶地址即新分配的内存起始地址
    - mmap()
    分配独立的大块内存
    当申请的内存超过最大值堆管理器用mmap()直接向内核申请一块独立的虚拟内存区域
    优点：释放的时候直接通过munmap()归还给内核，而brk是从堆顶收缩
    缺点：系统调用开销比brk大
    用户态堆管理器的核心实现？
    高效复用内存块，减少系统调用
    核心数据结构：内存块Chunk
        堆管理器把内核给分配的大块内存切割成一个个小的内存块Chunk
                    Chunk的头部：prev_size前一个Chunk的大小
                                size当前Chunk的大小
                                fd指向空闲链表中的下一个Chunk
                                bk指向空闲链表的前一个Chunk
                    可用内存区域：应用程序malloc的部分
                    size最低位标记前一个Chunl是否被占用1=占用，0=空闲
                        次低位标记是否是用mmap分配的Chunk1=是0=brk
        程序调用malloc(10)实际分配的Chunk大小会按8/16字节对齐
    核心管理策略：空闲链表
        堆管理器维护多个空闲链表
        - malloc:
            堆管理器先计算需要的Chunk总大小，n+头部+对齐
            遍历空闲链表，找最合适的Chunk,找到标记该Chunk为占用没找到则拆分更大的空闲Chunk,无任何空闲则向内核申请大块内存，切割后分配
        - free:
            free(ptr)，堆管理器先找到ptr对应的Chunk标记为空闲，然后合并相邻空闲的Chunk并加入对应大小的空闲链表，等待复用；如果是mmap分配的直接调用munmap()归还给内核
    解决内存碎片的核心手段：
        Chunk合并相邻空闲
        按大小分桶fastbin小Chunk,smallbin中Chunk,largebin大Chunk,避免遍历所有空闲Chunk
        内存对齐，8/16,避免不规则碎片
realloc底层：当前Chunk后面有空闲空间直接扩展=原地扩容，否则找新的Chunk拷贝原数据，释放旧的Chunk