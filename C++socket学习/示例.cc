//TCP服务器封装
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
class Socket {
    private:
     int fd_=-1;
    public:
    Socket(int domain,int type,int protocol=0){
        fd_ = ::socket(domain, type, protocol);
    }
    ~Socket() { 
        if(fd_!=-1){
            ::close(fd_);
            fd_ = -1;
        }
    }
    int fd() { 
        return fd_; 
    }
    bool isValid() { //检查socket是否有效
        return fd_ != -1;
     }
    bool enableReuseAddr() {//必须在socket之后，bind之前调用 
        int opt = 1;
        if(::setsockopt(fd_,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))==-1){
            std::cerr << strerror(errno) << std::endl;
            return false;
        }
        return true;
    }
};
class IPv4Address {
    private:
     sockaddr_in addr_;
    public:
     IPv4Address(uint16_t port) { 
        memset(&addr_, 0, sizeof(addr_));
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        addr_.sin_addr.s_addr = INADDR_ANY;
     }
     sockaddr* addr() { 
        return reinterpret_cast<sockaddr*>(&addr_);
     }
};
class TcpServer{
    private:
     uint16_t port_;
     Socket ServerSock_;
     void handleClient(int clientfd) {
         char buf[1024] = {0};
    while(1){
        ssize_t n = ::recv(clientfd, buf, sizeof(buf) - 1, 0);
        buf[n] = '\0';
        ::send(clientfd, buf, n, 0);
        ::close(clientfd);
    }
}
    public:
    TcpServer(uint16_t port):port_(port),ServerSock_(AF_INET,SOCK_STREAM){
        if(!ServerSock_.isValid()){
            exit(1);
        }
        ServerSock_.enableReuseAddr();
        IPv4Address addr(port_);
        ::bind(ServerSock_.fd(), addr.addr(), sizeof(addr));
        ::listen(ServerSock_.fd(), 10);
    }
    void start(){
        while(1){
            int clientfd = ::accept(ServerSock_.fd(), NULL, NULL);
            std::thread c1(&TcpServer::handleClient, this, clientfd);
            c1.detach();
        }
    }
};
int main(){
    TcpServer server(8080);
    server.start();
    return 0;
}