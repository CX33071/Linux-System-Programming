#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <string>
#include <unistd.h>
#include <sys/un.h>
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
    Socket Client_;
    int datafd;
    void sendmessage(std::string s) { 
        s += "\r\n";
        send(Client_.fd(), s.c_str(), s.size(),0);
     }
     std::string recvmessage() { 
        std::string ret;
        char c;
        while(read(Client_.fd(),&c,1)==1){
            ret += c;
            if(c=='\n'){
                break;
            }
        }
        std::cout << ret;
        return ret;
     }
     int getcode(std::string s) { 
        return std::stoi(s.substr(0, 3));
     }
     void connectdata(std::string ip, uint16_t port){       sockaddr_in addr{};
         addr.sin_family = AF_INET;
         addr.sin_port = htons(port);
         inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
         datafd = socket(AF_INET, SOCK_STREAM, 0);
         connect(datafd, (sockaddr*)&addr, sizeof(addr));
     }
     void prasePASV(std::string& ip,int &port,std::string s){
         int h1, h2, h3, h4, p1, p2;
         sscanf(s.c_str(), "227 entering passive mode (%d,%d,%d,%d,%d,%d)", &h1,
                &h2, &h3, &h4, &p1, &p2);
         ip = std::to_string(h1) + "." + std::to_string(h2) + "." +
              std::to_string(h3) + "." + std::to_string(h4);
         port = p1 * 256 + p2;
     }
    public:
    FTPClient(uint16_t port,std::string ip):Client_(AF_INET,SOCK_STREAM,0),datafd(-1){
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
        connect(Client_.fd(), (sockaddr*)&addr, sizeof(addr));
        recvmessage();
    }
    void PASV() { 
        sendmessage("PASV");
        std::string s = recvmessage();
        std::string ip;
        int port;
        prasePASV(ip, port, s);
        connectdata(ip, port);
    }
    void LIST() {
        PASV();
        sendmessage("LIST");
        std::string s=recvmessage();
        if(getcode(s)!=150){
            std::cerr << "LIST failed" << std::endl;
            close(datafd);
            datafd = -1;
            return;
        }
        ssize_t n;
        char buf[1024];
        while((n=read(datafd,buf,sizeof(buf)))>0){
            write(1, buf, n);
        }
        s=recvmessage();
        if(getcode(s)!=226){
            std::cerr << "LIST not completed" << std::endl;
        }
        close(datafd);
        datafd = -1;
    }
    void RETR(std::string s,std::string s2) {
        PASV();
        sendmessage(s);
        s=recvmessage();
        if (getcode(s) != 150) {
            std::cerr << "RETR failed" << std::endl;
            close(datafd);
            datafd = -1;
            return;
        }
        ssize_t n;
        char buf[1024];
        int fd = open(s2.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        while((n=read(datafd,buf,sizeof(buf)))>0){
            write(fd, buf, n);
        }
        s = recvmessage();
        if (getcode(s) != 226) {
            std::cerr << "RETR not completed" << std::endl;
        }
        close(fd);
        close(datafd);
        datafd = -1;
    }
    void STOR(std::string s, std::string s2) { 
        PASV();
        sendmessage(s);
        s = recvmessage();
        if (getcode(s) != 150) {
            std::cerr << "STOR failed" << std::endl;
            close(datafd);
            datafd = -1;
            return;
        }
        ssize_t n;
        char buf[1024];
        int fd = open(s2.c_str(), O_RDONLY);
        while((n=read(fd,buf,sizeof(buf)))>0){
            write(datafd, buf, n);
        }
        close(datafd);
        s = recvmessage();
        if (getcode(s) != 226) {
            std::cerr << "STOR not completed" << std::endl;
        }
        close(fd);
        datafd = -1;
    }
    void start() { 
        std::string s,s1,s2;
        while(1){
            std::getline(std::cin, s);
            int idx = s.find(' ');
            if(idx==std::string::npos){
                s1 = s;
                s2 = "";
            }else{
            s1 = s.substr(0, idx);
            s2 = s.substr(idx+1);
            }
            if (!strcasecmp(s1.c_str(), "PASV")) {
                PASV();
            } else if (!strcasecmp(s1.c_str(), "LIST")) {
                LIST();
            } else if (!strcasecmp(s1.c_str(), "RETR")) {
                RETR(s,s2);
            } else if (!strcasecmp(s1.c_str(), "STOR")) {
                STOR(s,s2);
            }
        }
    }
};
int main(int argc,char*argv[]){
    std::string ip = argv[1];
    FTPClient ftpClient(2100,ip);
    ftpClient.start();
    return 0;
}