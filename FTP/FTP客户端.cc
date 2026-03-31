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
     Socket Client_;
    public:
    FTPClient(uint16_t port):Client_(AF_ALG,SOCK_CLOEXEC,0) { 
        IPV4 addr_(port);
        connect(Client_.fd(), addr_.change(), addr_.len());

    }
    int recvmessage(std::string& s) {
        std::string ret;
        ssize_t n;
        for (char c : s) {
            if (c == '\n') {
                break;
            }
            ret += c;
            n++;
        }
        return n;
    }
    void sendmessage(std::string& cmd) {
        cmd += "\r\n";
        write(Client_.fd(), cmd.c_str(), cmd.size());
    }
    void prasePASV(std::string& ip, std::string s, uint16_t& port) {
        int h1, h2, h3, h4, p1, p2;
        sscanf(s.c_str(), "entering passive mode (%d,%d,%d,%d,%d,%d)", h1, h2,
               h3, h4, p1, p2);
        port = p1 * 256 + p2;
        ip += std::to_string(h1);
        ip += '.';
        ip += std::to_string(h2);
        ip += '.';
        ip += std::to_string(h3);
        ip += '.';
        ip += std::to_string(h4);
    }
    void createconnect(std::string s) {
        std::string ip;
        uint16_t port;
        prasePASV(ip, s, port);
        sockaddr_in addr_ = {};
        addr_.sin_family = AF_INET;
        addr_.sin_port = port;
        inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr);
        connect(Client_.fd(), reinterpret_cast<sockaddr*>(&addr_),
                sizeof(addr_));
    }
    void LIST() {}
    void RETR(std::string file) {

    }
    void STOR() {}
    int getfd() {
         return Client_.fd(); 
        }
};
int main(){
    FTPClient Client_(2100);
    while(1){
        std::string s;
        std::cin >> s;
        if(!strcasecmp(s.c_str(),"PASV")){
            std::string s = "PASV";
            Client_.sendmessage(s);
            Client_.recvmessage(s);
            Client_.createconnect(s);
        } else if (!strcasecmp(s.c_str(), "LIST")) {
            std::string s = "LIST";
            Client_.sendmessage(s);
            ssize_t n;
            char buf[1024] = {};
            while ((n = Client_.recvmessage(s)) > 0) {
                write(Client_.getfd(), buf, sizeof(buf));
            }
        } else if (!strcasecmp(s.c_str(), "retr")) {
            std::string file;
            std::cin >> file;
            std::string cmd;
            cmd = s;
            cmd += ' ';
            cmd += file;
            ssize_t n;
            Client_.sendmessage(cmd);
            int fd = open(file.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
            while((n=Client_.recvmessage(cmd))>0){
                write(Client_.getfd(),buf,sizeof(buf))
            }
        } else if (!strcasecmp(s.c_str(), "STOR")) {
            std::string file;
            std::cin >> file;
            std::string cmd;
            cmd = s;
            cmd += ' ';
            cmd += file;
            Client_.sendmessage(cmd);
        }
    }
    return 0;
}