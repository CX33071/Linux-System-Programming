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
class Socket{
   private:
    int fd_ = -1;
   public:
    Socket(int domain, int type, int protocol);
    ~Socket();
    int fd();
    bool isvalid();
    bool enableport();
};





Socket::Socket(int domain, int type, int protocol) {
    fd_ = socket(domain, type, protocol);
}
Socket::~Socket() {
    if (fd_ != -1) {
        close(fd_);
        fd_ = -1;
    }
}
int Socket::fd() {
    return fd_;
}
bool Socket::isvalid() {
    return fd_ != -1;
}
bool Socket::enableport() {
    int opt = 1;
    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cout << strerror(errno) << std::endl;
        return false;
    }
    return true;
}





class IPV4{
    private:
     sockaddr_in addr{};
    public:
     IPV4(uint16_t port);
     sockaddr* change();
     socklen_t len();
     uint16_t getport();
};





IPV4::IPV4(uint16_t port) {
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
}
sockaddr* IPV4::change() {
    return (sockaddr*)&addr;
}
socklen_t IPV4::len() {
    return sizeof(addr);
}
uint16_t IPV4::getport() {
    return ntohs(addr.sin_port);
}




class Ftpserver {
   private:
    uint16_t port_;
    Socket listensock_;
    std::string readline(int fd);
    std::string getIP(IPV4 addr);
    void sendmessage(int fd, std::string num, std::string s);
    int createPASVsocket(int& datafd,IPV4 &addr);
    void LIST(int cfd, int datafd);
    void RETR(int cfd, int datafd, std::string s);
    void STOR(int cfd, int datafd, std::string s);
    void handleclient(int cfd);
    public:
     Ftpserver(uint16_t port);
};




std::string Ftpserver::readline(int fd) {
    std::string line;
    char c;
    while (1) {
        ssize_t n = read(fd, &c, 1);
        if (n <= 0) {
            break;
        }
        line += c;
        if (c == '\n') {
            break;
        }
    }
    return line;
}
std::string Ftpserver::getIP(IPV4 addr) {
    socklen_t len = addr.len();
    if (getsockname(listensock_.fd(), addr.change(), &len)) {
        
    }
}
void Ftpserver::sendmessage(int fd, std::string num, std::string s) {
    std::string message;
    message += num;
    message += ' ';
    message += s;
    message += "\r\n";
    write(fd, message.data(), message.size());
}
int Ftpserver::createPASVsocket(int& datafd,IPV4 &addr) {
    datafd = socket(AF_INET, SOCK_STREAM, 0);
    bind(datafd, addr.change(), addr.len());
    listen(datafd, 1);
    socklen_t len = addr.len();
    getsockname(datafd, addr.change(), &len);
    int port = addr.getport();
    return port;
}
void Ftpserver::LIST(int cfd, int datafd) {
    int dfd = accept(datafd, nullptr, nullptr);
    sendmessage(cfd, "150", "List");
    DIR* dir = opendir(".");
    struct dirent* ent;
    while (ent = readdir(dir)) {
        std::string s = ent->d_name;
        s += "\r\n";
        write(dfd, s.data(), s.size());
    }
    closedir(dir);
    close(dfd);
    sendmessage(cfd, "226", "List done");
    close(datafd);
    datafd = -1;
}
void Ftpserver::RETR(int cfd, int datafd, std::string s) {
    int dfd = accept(datafd, nullptr, nullptr);
    int fd = open(s.data(), O_RDONLY);
    if (fd == -1) {
        sendmessage(cfd, "550", "File not found");
        close(dfd);
        return;
    }
    sendmessage(cfd, "150", "Send file");
    char buf[1024];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        write(dfd, buf, n);
    }
    close(fd);
    close(dfd);
    sendmessage(cfd, "226", "Download done");
    close(datafd);
    datafd = -1;
}
void Ftpserver::STOR(int cfd, int datafd, std::string s) {
    int dfd = accept(datafd, nullptr, nullptr);
    sendmessage(cfd, "150", "Receive file");
    int fd = open(s.data(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    char buf[1024];
    ssize_t n;
    while ((n = read(dfd, buf, sizeof(buf))) > 0) {
        write(fd, buf, n);
    }
    close(fd);
    close(dfd);
    sendmessage(cfd, "226", "Upload done");
    close(datafd);
    datafd = -1;
}
void Ftpserver::handleclient(int cfd) {
    int datafd = -1;
    std::string clientip;
    char c;
    // while (read(cfd, &c, 1) == 1) {
    //     clientip += c;
    //     if (c == '\n') {
    //         break;
    //     }
    // }
    //     write(listensock_.fd(), clientip.data(), clientip.size());
    sendmessage(cfd, "220", "FTP server ready");
    while (1) {
        std::string s = readline(cfd);
        if (s.empty()) {
            break;
        }
        std::string s1, s2;
        size_t space = s.find(' ');
        if (!s.empty() && s.back() == '\n') {
            s.pop_back();
        }
        if (!s.empty() && s.back() == '\r') {
            s.pop_back();
        }
        if (space == std::string::npos) {
            s1 = s;
            s2 = "";
        } else {
            s1 = s.substr(0, space);
            s2 = s.substr(space + 1);
        }
        if (!strcasecmp(s1.c_str(), "pasv")) {
            IPV4 addr(0);
            int port = createPASVsocket(datafd,addr);
            int p1 = port / 256;
            int p2 = port % 256;
            char buf[1024];
            std::string ip = getIP(addr);
            std::string s = "entering passive mode (";
            s += ip;
            s += ',';
            s += std::to_string(p1);
            s += ',';
            s += std::to_string(p2);
            s += ')';
            sendmessage(cfd, "227", s);
            // std::string ret = "主动模式已开启";
            // std::cout << ret;
        } else if (!strcasecmp(s1.c_str(), "list")) {
            LIST(cfd, datafd);
        } else if (!strcasecmp(s1.c_str(), "retr")) {
            RETR(cfd, datafd, s2);
        } else if (!strcasecmp(s1.c_str(), "stor")) {
            STOR(cfd, datafd, s2);
        }
    }
    close(cfd);
}
Ftpserver::Ftpserver(uint16_t port)
    : port_(port), listensock_(AF_INET, SOCK_STREAM, 0) {
    listensock_.enableport();
    IPV4 addr(port_);
    bind(listensock_.fd(), addr.change(), addr.len());
    listen(listensock_.fd(), 10);
    std::cout << "服务端已开启" << std::endl;
    while (1) {
        int cfd = accept(listensock_.fd(), nullptr, nullptr);
        // std::string ret = "有一个客户端连接成功";
        // std::cout << ret;
        std::thread t(&Ftpserver::handleclient, this, cfd);
        t.detach();
    }
}
int main(){
    Ftpserver server(2100);
    return 0;
}
