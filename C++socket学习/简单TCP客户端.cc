#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
class Socket {
    private:
     int fd_ = -1;
     public:
    Socket(int domain,int type,int protocol){
        fd_ = ::socket(domain, type, protocol);
    }
    ~Socket(){
        if(fd_==-1){
            close(fd_);
            fd_= -1;
        }
    }
    int fd() { return fd_; }
    bool isValid() { return fd_ != -1; }
};
class IPv4Address{
    private:
     sockaddr_in addr_;
     public:
      IPv4Address(uint16_t port) { 
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        addr_.sin_addr.s_addr = INADDR_ANY;
      }
      sockaddr* change() { 
        return reinterpret_cast<sockaddr*>(&addr_);
     }
};
class Client{
    private:
     Socket clientsocket_;
     IPv4Address serveraddr_;
    public:
     Client(IPv4Address&serveraddr):clientsocket_(AF_INET,SOCK_STREAM,0),serveraddr_(serveraddr){}
     void connect(){
         ::connect(clientsocket_.fd(), serveraddr_.change(),
                   sizeof(serveraddr_));
     }
     void send(char* data, int size) { 
        ::send(clientsocket_.fd(), data,size,0); 
    }
    void recv(char* buf, int size) { ::recv(clientsocket_.fd(), buf, size, 0); }
};
int main() {
    // // 1. 先创建地址
    // IPv4Address addr("127.0.0.1", 8080);

    // // 2. 直接传给客户端！
    // Client client(addr);

    // client.connect();
    // client.send("hello", 5);

    // char buf[1024] = {0};
    // client.recv(buf, sizeof(buf) - 1);
    // std::cout << "recv: " << buf << std::endl;

    // return 0;
}
