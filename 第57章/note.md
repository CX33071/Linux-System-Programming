同一主机系统上进程之间的相互通信
用本地文件系统的路径在作为地址
同一主机是因为这两个socket都是与本地文件系统路径绑定
1.UNIX domain socket地址：struct sockaddr_un
    struct sockaddr_un{
        sa_family_t sun_family;
        char_sun_path[108];
    }
    socket地址以路径名来表示，UNIX domain socket绑定的是一个本地文件系统路径，只在本地通信，客户端通过这个路径找到你的socket
绑定步骤：先初始化一个sockaddr_un结构，strncpy复制地址给结构体赋值，然后bind
绑定UNIX domain socket时，bind会在文件系统中创建一个条目，ls -l时第一列会显示s
注意：
    无法将一个socket绑定到一个既有路径名上，bind会返回失败，所以绑定前需要先删除旧的socket
    通常将一个socket绑定到一个绝对路径名上，这个socket就会位于文件系统中的一个固定地址处
    一个socket只能绑定一个路径名，一个路径名也只能被一个socket绑定
    无法用open打开一个socket
    不再需要一个socket时，可以使用unlink删除其路径名目条
2.UNIX domain socket权限
socket文件的权限决定了哪些进程能够与这个socket进行通信
- 要连接一个UNIX domain流 socket需要在这个socket文件上拥有写权限
- 要通过一个UNIX domain数据报socket发送一个数据报需要在该socket文件上拥有写权限
- 此外，需要在存放socket路径名的所有目录上都拥有执行权限，通常创建socket时会赋予所有权限，可以通过umask限制权限
3.创建互联socket对：socketpair()
是创建socket并连接的快捷方式
int socketpair(int domain,int type,int protocol,int sockfd[2]);
domain参数必须是AF_UNIX,protocol参数为0
4.linux抽象socket空间
允许一个UNIX domain socket绑定到一个名字上但不会在文件系统中创建该名字
优势：无需担心文件名字冲突，不用在socket使用完后删除文件路径
要创建一个抽象绑定就要将sun_path字段的第一个字节指定为null,与传统绑定区分开来
客户端不是 “没有地址”，而是 “不需要手动绑定地址，内核会自动为它分配一个匿名 / 临时地址”
5.UNIX domain socket流
客户端的输入被传给服务器，服务器把这个东西作为标准输出，也就是客户端输入什么，服务器输出什么
服务器的监听 socket（sfd）永远不会和客户端 socket 直接连接，它只做 “接单员”；真正和客户端 socket 建立连接、传输数据的，是服务器通过accept()新创建的通信 socket（cfd）
6.网络传输的数据报不可靠，UNIX domain 数据报socket可靠
✅ exit() 会让内核自动关闭进程打开的所有文件描述符（包括 socket 的sfd），所以从功能上看，exit 前不写close(sfd)也不会导致 socket 资源泄漏；❌ 但强烈建议先手动close(sfd)（尤其是 UNIX domain socket），因为exit()不会自动删除你手动bind的 socket 文件（如/tmp/ud_ucase_cl.12345），会导致下次运行绑定失败
