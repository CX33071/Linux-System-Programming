#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <thread>
class Socket {
   private:
    int fd_ = -1;

   public:
    Socket(int domain, int type, int protocol) {
        fd_ = socket(domain, type, protocol);
    }
    ~Socket() {
        if (fd_ != -1) {
            close(fd_);
            fd_ = -1;
        }
    }
    int fd() { return fd_; }
    bool isvalid() { return fd_ != -1; }
    bool enableport() {
        int opt = 1;
        if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ==
            -1) {
            std::cout << strerror(errno) << std::endl;
            return false;
        }
        return true;
    }
};
class IPV4 {
   private:
    sockaddr_in addr{};

   public:
    IPV4(uint16_t port) {
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
    }
    sockaddr* change() { return (sockaddr*)&addr; }
    socklen_t len() { return sizeof(addr); }
    uint16_t getport() { return ntohs(addr.sin_port); }
};
class FTPClient{
    private:
     int cfd_;
     std::string recvmessage() { 
        std::string s;
        char c;
        while(read(cfd_,&c,1)==1){
            s += c;
            if(c=='\n'){
                break;
            }
        }
        return s;
     }
     void sendmessage(std::string cmd) { 
        std::string s = cmd + "\r\n";
        write(cfd_, s.data(), s.size());
     }
     void parsePASV(std::string&s,std::string&ip,int&port){
         int h1, h2, h3, h4, p1, p2;
         sscanf(s.c_str(), "227 entering passive mode (%d,%d,%d,%d,%d,%d)", &h1,
                &h2, &h3, &h4, &p1, &p2);
         ip = std::to_string(h1) + "." + std::to_string(h2) + "." +
              std::to_string(h3) + "." + std::to_string(h4);
         port = p1 * 256 + p2;
     }
     int connectdata(std::string ip,int port){
         int fd = socket(AF_INET, SOCK_STREAM, 0);
         sockaddr_in addr{};
         addr.sin_family = AF_INET;
         addr.sin_port = htons(port);
         inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
         connect(fd, (sockaddr*)&addr, sizeof(addr));
         return fd;
     }

    public:
     FTPClient(std::string ip,uint16_t port) {
         cfd_ = socket(AF_INET, SOCK_STREAM, 0);
         sockaddr_in addr{};
         addr.sin_family = AF_INET;
         addr.sin_port = htons(port);
         inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
         connect(cfd_, (sockaddr*)&addr,sizeof(addr));
         std::cout << recvmessage();
     }
     void LIST() { 
        sendmessage("PASV");
        std::string s = recvmessage();
        std::cout << s;
        std::string ip;
        int port;
        parsePASV(s, ip, port);
        int dfd = connectdata(ip, port);
        sendmessage("LIST");
        std::cout << recvmessage();
        char buf[1024];
        ssize_t n;
        while((n=read(dfd,buf,sizeof(buf)))>0){
            write(1, buf, n);
        }
        close(dfd);
        std::cout << recvmessage();
     }
     void RETR(std::string file) {
             sendmessage("PASV");
             std::string s = recvmessage();
             std::cout << s;
             std::string ip;
             int port;
             parsePASV(s, ip, port);
             int dfd = connectdata(ip, port);
             sendmessage("RETR " + file);
             std::cout << recvmessage();
             int fd =
                 open(file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
             ssize_t n;
             char buf[1024];
             while ((n = read(dfd, buf, sizeof(buf))) > 0) {
                 write(fd, buf, n);
             }
     }
     void STOR(std::string file) {
         sendmessage("PASV");
         std::string s = recvmessage();
         std::cout << s;
         std::string ip;
         int port;
         parsePASV(s, ip, port);
         int dfd = connectdata(ip, port);
         sendmessage("STOR " + file);
         std::cout << recvmessage();
         int fd = open(file.c_str(), O_RDONLY);
         ssize_t n;
         char buf[1024];
         while ((n = read(fd, buf, sizeof(buf))) > 0) {
             write(dfd, buf, n);
         }
     }
};
int main() {
    FTPClient client("127.0.0.1", 2100);
        while (true) {}
        return 0;
}