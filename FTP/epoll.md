1.epoll
IO多路复用器，用来监听大量socket/文件描述符的动态，监控事件，返回有事件发生的描述符
原理：红黑树管理所有注册的fd，链表存放就绪的fd。epoll_wait直接读取，无需遍历所有fd
EPOLL有两个核心模式：LT水平触发(默认模式)——ET边缘触发(高性能模式)
    LT：只要缓冲区有数据就一直通知，一次没把数据读完也没事，下次epoll_wait还会提醒
    ET：只有fd监控的事件状态有变化的时候才通知，并且只通知一次，必须一次性必须把数据读完，否则数据留在缓冲区，再也收不到通知
！！！ET模式必须和非阻塞IO一起使用，用阻塞IO循环read读取，当数据读完的时候read会继续等待数据读取，read阻塞，线程僵死，用非阻塞IO,read读完数据之后会返回-1,错误码EAGAIN/EWOULDBLOCK，退出循环即可
2.ET边缘触发的正确读写：
    char buf[4096];
    while(1){
        int n=read(fd,buf,sizeof(buf));
        if(n>0){
            处理数据
        }else if(n==0){//对端关闭
            close(fd);
            break;
        }else{
            if(errno==EAGAIN||errno==EWOULDBLOCK){
                数据读完了，退出
                break;
            }
            真正错误
            close(fd);
            break;
        }
    }
3.注意必须把监听的socket设置为非阻塞
    int flag=fcntl(fd,F_GETFL,0);
    fcntl(fd,F_SETFL,flag|O_NONBLOCK);
4.epoll可监控的全部事件：
    EPOLLIN         读事件      TCP有数据到达、UDP收到包、管道/套接字对端发送数据
    EPOLLOUT        写事件      发送缓冲区有空位，可以发送数据
    EPOLLERR        错误事件    连接出错，内核自动监听，不用手动添加
    EPOLLHUP        挂起事件    对端关闭连接/管道关闭，内核自动舰艇，不用手动添加
    EPOLLET         边缘触发模式    不写默认LT
    EPOLLONESHOT    一次性触发      触发一次后失效，避免同一个事件北京多个线程处理
5.注意：EPOLLOUT不能一直监听
当发送缓冲区要发送的数据没写满时就一直触发写事件，通知现在发送缓冲区可写
什么时候用EPOLLOUT？
要发数据，调用write，缓冲区满了返回EAGAIN，此时注册EPOLLOUT，等内核通知缓冲区有空位了继续发数据，发完立马删除EPOLLOUT
总结：不要一直监听EPOLLOUT，写不进去了缓冲区满了才监听，写完立刻取消
  注意：EPOLLONESHOT=让一个socket事件同一时间只被一个线程处理，防止并发混乱
6.用发送缓冲区，非阻塞不能保证一次send发完所有数据，完一发送缓冲区满了，剩下没发的就被丢弃了