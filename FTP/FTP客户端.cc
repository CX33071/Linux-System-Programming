#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <limits>
class Socket {
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

class IPV4 {
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


class FTPClient{
   private:
    Socket Client_;
    int datafd;
    std::string serverip;
    void sendmessage(std::string s);
    std::string recvmessage();
    int getcode(std::string s);
    void connectdata(std::string ip, uint16_t port);
    void prasePASV(std::string& ip, int& port, std::string s);
    std::string getip(sockaddr_in addr);
   public:
    FTPClient(uint16_t port,std::string ip);
    void PASV();
    void LIST();
    void RETR(std::string s, std::string s2);
    void STOR(std::string s, std::string s2);
    void start();
};


std::string FTPClient::getip(sockaddr_in addr){
    char ip[100];
    socklen_t len = sizeof(addr);
    getsockname(Client_.fd(),(sockaddr*)&addr,&len);
    inet_ntop(AF_INET, &addr, ip, sizeof(ip));
    std::string ret = ip;
    return ret;
}
void FTPClient::sendmessage(std::string s) {
    s += "\r\n";
    send(Client_.fd(), s.c_str(), s.size(), 0);
}
std::string FTPClient::recvmessage() {
    std::string ret;
    char c;
    while (read(Client_.fd(), &c, 1) == 1) {
        ret += c;
        if (c == '\n') {
            break;
        }
    }
    return ret;
}
int FTPClient::getcode(std::string s) {
    return std::stoi(s.substr(0, 3));
}
void FTPClient::connectdata(std::string ip, uint16_t port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    datafd = socket(AF_INET, SOCK_STREAM, 0);
    connect(datafd, (sockaddr*)&addr, sizeof(addr));
}
void FTPClient::prasePASV(std::string& ip, int& port, std::string s) {
    int h1, h2, h3, h4, p1, p2;
    sscanf(s.c_str(), "227 entering passive mode (%d,%d,%d,%d,%d,%d)", &h1, &h2,
           &h3, &h4, &p1, &p2);
    ip = std::to_string(h1) + "." + std::to_string(h2) + "." +
         std::to_string(h3) + "." + std::to_string(h4);
    port = p1 * 256 + p2;
}
FTPClient::FTPClient(uint16_t port, std::string ip)
    : Client_(AF_INET, SOCK_STREAM, 0), datafd(-1) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr.s_addr);
    connect(Client_.fd(), (sockaddr*)&addr, sizeof(addr));
    std::string IP = getip(addr);
    sendmessage(IP);
    std::string ret=recvmessage();
    std::cout << ret;
}
void FTPClient::PASV() {
    std::cout << "123";
    sendmessage("PASV");
    std::string s = recvmessage();
    std::cout << s;
    std::string ip;
    int port;
    prasePASV(ip, port, s);
    connectdata(ip, port);
}
void FTPClient::LIST() {
    if(datafd==-1){
        PASV();
    }
    sendmessage("LIST");
    std::string s = recvmessage();
    if (getcode(s) != 150) {
        std::cerr << "LIST failed" << std::endl;
        close(datafd);
        datafd = -1;
        return;
    }
    ssize_t n;
    char buf[1024];
    while ((n = read(datafd, buf, sizeof(buf))) > 0) {
        write(1, buf, n);
    }
    s = recvmessage();
    if (getcode(s) != 226) {
        std::cerr << "LIST not completed" << std::endl;
    }
    close(datafd);
    datafd = -1;
}
void FTPClient::RETR(std::string s, std::string s2) {
    if(datafd==-1){
        PASV();
    }
    sendmessage(s);
    s = recvmessage();
    if (getcode(s) != 150) {
        std::cerr << "RETR failed" << std::endl;
        close(datafd);
        datafd = -1;
        return;
    }
    ssize_t n;
    char buf[1024];
    int fd = open(s2.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    while ((n = read(datafd, buf, sizeof(buf))) > 0) {
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
void FTPClient::STOR(std::string s, std::string s2) {
    if(datafd==-1){
        PASV();
    }
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
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
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

//如何记录注册的章好和密码
//为什么while(1)里std::cin>>s不能输入
void FTPClient::start() {
    std::string s, s1, s2, ret;
    std::cout << "注册L/登录S:";
    std::string c;
    std::cin >> c;
    sendmessage(c);
    if (c == "L") {
        std::cout << "请输入用户名:";
        std::string name;
        std::cin >> name;
        sendmessage(name);
        std::cout << "请输入密码:";
        std::string pass;
        std::cin >> pass;
        sendmessage(pass);
        std::cout << "注册成功，请开始你的操作！\n";
    } else {
        std::cout << "请输入用户名:";
        while (ret != "请输入密码:\n") {
            s1 = "USER";
            std::cin >> s;
            s1 += s;
            sendmessage(s1);
            ret = recvmessage();
            std::cout << ret;
        }
        std::cout << "请输入密码:";
        while (ret != "成功登录\n") {
            s2 = "PASS";
            std::cin >> s;
            s2 += s;
            sendmessage(s2);
            ret = recvmessage();
            std::cout << ret << '\n';
        }
    }
    while (1) {
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::string cmd;
        if(!std::getline(std::cin, cmd)){
            break;
        }
        int idx = cmd.find(' ');
        if (idx == std::string::npos) {
            s1 = cmd;
            s2 = "";
        } else {
            s1 = cmd.substr(0, idx);
            s2 = cmd.substr(idx + 1);
        }
        if (!strcasecmp(s1.c_str(), "PASV")) {
            std::cout << "5";
            PASV();
        } else if (!strcasecmp(s1.c_str(), "LIST")) {
            LIST();
        } else if (!strcasecmp(s1.c_str(), "RETR")) {
            RETR(s1, s2);
        } else if (!strcasecmp(s1.c_str(), "STOR")) {
            STOR(s1, s2);
        } else {
            std::cout << "命令错误\n";
        }
    }
}
int main(int argc,char*argv[]){
    std::string ip = argv[1];
    FTPClient ftpClient(2100,ip);
    ftpClient.start();
    return 0;
}
