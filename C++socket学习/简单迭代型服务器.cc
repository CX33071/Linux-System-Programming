// TCP服务器封装
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <thread>
class Socket {
   private:
    int fd_ = -1;

   public:
    Socket(int domain, int type, int protocol) {
        fd_ = ::socket(domain, type, protocol);
    }
    ~Socket() {
        if (fd_ != -1) {
            ::close(fd_);
            fd_ = -1;
        }
    }
    int fd() { return fd_; }
    bool isValid() {  // 检查socket是否有效
        return fd_ != -1;
    }
    bool enableReuseAddr() {  // 必须在socket之后，bind之前调用
        int opt = 1;
        if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ==
            -1) {
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
    sockaddr* change() { return reinterpret_cast<sockaddr*>(&addr_); }
};
class TcpServer {
   private:
    uint16_t port_;
    Socket ServerSock_;
   public:
    TcpServer(uint16_t port)
        : port_(port), ServerSock_(AF_INET, SOCK_STREAM, 0) {
        if (!ServerSock_.isValid()) {
            exit(1);
        }
        ServerSock_.enableReuseAddr();
        IPv4Address addr(port_);
        ::bind(ServerSock_.fd(), addr.change(), sizeof(addr));
        ::listen(ServerSock_.fd(), 10);
        while(1){
            int cfd = ::accept(ServerSock_.fd(), NULL, NULL);
            if(cfd==-1){
                std::cout << strerror(errno) << std::endl;
                exit(1);
            }
            char buf[1024] = {0};
            while(1){
            ssize_t n = ::recv(cfd, buf, sizeof(buf)-1,0);
            if(n<=0){
                break;
            }
            buf[n] = '\0';
            ::send(cfd, buf, n, 0);
        }
        ::close(cfd);
        }
    }
};
int main() {
    TcpServer server(8080);
    return 0;
}