#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <sys/epoll.h>
#define MAX_SIZE 100

class Socket {
public:
    Socket(int domain, int type, int protocol);
    ~Socket();
    int fd();
    bool isvalid();
    bool enableport();

   private:
    int fd_ = -1;
};

class IPV4 {
public:
    IPV4(uint16_t port);
    sockaddr* change();
    socklen_t len();
    uint16_t getport();

private:
    sockaddr_in addr_{};
};

struct Client {
    bool hasuser = false;
    bool islogin = false;
    std::string username;
    int pasvfd = -1;
};
class Threadpool {

};
class ftpepollserver {
   public:
    ftpepollserver(uint16_t port);
    ~ftpepollserver();
    void run();

   private:
    void add_epoll(int fd, uint32_t events);
    void del_epoll(int fd, uint32_t events);
    void handleclient(int cfd);
    void USER(int cfd, Client& client, std::string& user);
    void PASS(int cfd, Client& client, std::string& pass);
    void PASV(int cfd, Client& client);
    int acceptdata(Client& client);
    void LIST(int cfd, Client& client);
    void RETR(int cfd, Client& client, std::string& path);
    void STOR(int cfd, Client& client, std::string& path);
    void closePASV(Client& client);

    uint16_t port_;
    Socket listensock_;
    int epfd;
    Threadpool pool;
    std::map<std::string, std::string> account;
};

std::string getlineok(std::string line);
std::string readline(int fd);
bool sendn(int fd, char* data, size_t len);
bool sendstr(int fd, std::string& text);
void sendmessage(int fd, int code, std::string text);
void set_nonblock(int fd);
Socket::Socket(int domain, int type, int protocol)
    : fd_(socket(domain, type, protocol)) {}

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

IPV4::IPV4(uint16_t port) {
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_.sin_port = htons(port);
}

sockaddr* IPV4::change() {
    return reinterpret_cast<sockaddr*>(&addr_);
}

socklen_t IPV4::len() {
    return sizeof(addr_);
}

uint16_t IPV4::getport() {
    return ntohs(addr_.sin_port);
}

ftpepollserver::ftpepollserver(uint16_t port)
    : port_(port), listensock_(AF_INET, SOCK_STREAM, 0) {
    account["ftp"] = "123456";
    set_nonblock(listensock_.fd());
    epfd = epoll_create1(0);
}
ftpepollserver::~ftpepollserver(){
    close(epfd);
}
void ftpepollserver::run() {
    if (!listensock_.isvalid()) {
        std::perror("socket");
        return;
    }
    if (!listensock_.enableport()) {
        std::perror("setsockopt");
        return;
    }

    IPV4 addr(port_);
    bind(listensock_.fd(), addr.change(), addr.len());
    listen(listensock_.fd(), 16);
    std::cout << "ftp server listen" << port_ << '\n';
    add_epoll(listensock_.fd(), EPOLLET | EPOLLIN);
    struct epoll_event events[MAX_SIZE];
    while (1) {
        int n = epoll_wait(epfd, events, MAX_SIZE, -1);
        for (int i = 0; i < n;i++){
            int fd = events[i].data.fd;
            if(fd==listensock_.fd()){
                while(1){
                int cfd = accept(listensock_.fd(), nullptr, nullptr);
                std::cout << "有一个客户端成功连接\n";
                set_nonblock(cfd);
                add_epoll(cfd, EPOLLIN | EPOLLET);
               
            }
         } else {
            if(events[i].events&EPOLLIN){
                
            }
            }
        }
    }
    close(epfd);
}

void ftpepollserver::handleclient(int cfd) {
    Client client;
    sendmessage(cfd, 220, "ftp server ready");

    while (1) {
        std::string s = readline(cfd);
        std::string cmd;
        std::string arg;
        size_t pos = s.find(' ');
        if (pos == std::string::npos) {
            cmd = s;
        } else {
            cmd = s.substr(0, pos);
            arg = s.substr(pos + 1);
        }
        if (cmd == "USER") {
            USER(cfd, client, arg);
        } else if (cmd == "PASS") {
            PASS(cfd, client, arg);
        } else if (cmd == "QUIT") {
            sendmessage(cfd, 221, "bye bye");
            break;
        } else if (!client.islogin) {
            sendmessage(cfd, 530, "login first");
        } else if (!strcasecmp(cmd.c_str(), "PASV")) {
            PASV(cfd, client);
        } else if (!strcasecmp(cmd.c_str(), "LIST")) {
            LIST(cfd, client);
        } else if (!strcasecmp(cmd.c_str(), "RETR")) {
            RETR(cfd, client, arg);
        } else if (!strcasecmp(cmd.c_str(), "STOR")) {
            STOR(cfd, client, arg);
        } else {
            sendmessage(cfd, 502, "command errno");
        }
    }

    closePASV(client);
    close(cfd);
}

void ftpepollserver::USER(int cfd, Client& client, std::string& user) {
    if (user.empty()) {
        sendmessage(cfd, 501, "username");
        return;
    }
    if (account.count(user) == 0) {
        sendmessage(cfd, 530, "username exist");
        return;
    }
    client.username = user;
    client.hasuser = true;
    client.islogin = false;
    sendmessage(cfd, 331, "need password");
}

void ftpepollserver::PASS(int cfd, Client& client, std::string& pass) {
    if (!client.hasuser) {
        sendmessage(cfd, 503, "need username");
        return;
    }
    auto it = account.find(client.username);
    if (it == account.end() || it->second != pass) {
        sendmessage(cfd, 530, "password errno");
        return;
    }
    client.islogin = true;
    sendmessage(cfd, 230, "login succeful");
}

void ftpepollserver::PASV(int cfd, Client& client) {
    closePASV(client);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    IPV4 addr(0);

    bind(fd, addr.change(), addr.len());
    listen(fd, 1);
    socklen_t len = addr.len();
    getsockname(fd, addr.change(), &len);
    sockaddr_in caddr{};
    len = sizeof(caddr);
    getsockname(cfd, reinterpret_cast<sockaddr*>(&caddr), &len);
    unsigned char* ip =
        reinterpret_cast<unsigned char*>(&caddr.sin_addr.s_addr);
    uint16_t port = addr.getport();
    int p1 = port / 256;
    int p2 = port % 256;

    client.pasvfd = fd;
    sendmessage(cfd, 227,
                "Entering Passive Mode (" + std::to_string(ip[0]) + "," +
                    std::to_string(ip[1]) + "," + std::to_string(ip[2]) + "," +
                    std::to_string(ip[3]) + "," + std::to_string(p1) + "," +
                    std::to_string(p2) + ")");
}

int ftpepollserver::acceptdata(Client& client) {
    if (client.pasvfd == -1) {
        return -1;
    }
    int datafd = accept(client.pasvfd, nullptr, nullptr);
    closePASV(client);
    return datafd;
}

void ftpepollserver::LIST(int cfd, Client& client) {
    if (client.pasvfd == -1) {
        sendmessage(cfd, 425, "need pasv first");
        return;
    }
    sendmessage(cfd, 150, "open data connection for list");
    int datafd = acceptdata(client);
    if (datafd == -1) {
        sendmessage(cfd, 425, "data connect failed");
        return;
    }

    DIR* dir = opendir(".");
    if (dir == nullptr) {
        close(datafd);
        sendmessage(cfd, 550, "open dir failed");
        return;
    }

    dirent* ent = nullptr;
    while ((ent = readdir(dir)) != nullptr) {
        std::string line = std::string(ent->d_name) + "\r\n";
        if (!sendstr(datafd, line)) {
            break;
        }
    }
    closedir(dir);
    close(datafd);
    sendmessage(cfd, 226, "list complete");
}

void ftpepollserver::RETR(int cfd, Client& client, std::string& path) {
    if (path.empty()) {
        sendmessage(cfd, 501, "need file name");
        return;
    }
    if (client.pasvfd == -1) {
        sendmessage(cfd, 425, "need pasv first");
        return;
    }

    int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        sendmessage(cfd, 550, "file not found");
        return;
    }

    sendmessage(cfd, 150, "open data connect for retr");
    int datafd = acceptdata(client);
    if (datafd == -1) {
        close(fd);
        sendmessage(cfd, 425, "data connect failed");
        return;
    }

    char buf[4096];
    ssize_t n = 0;
    bool ok = true;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        if (!sendn(datafd, buf, static_cast<size_t>(n))) {
            ok = false;
            break;
        }
    }

    close(fd);
    close(datafd);
    int num = ok ? 226 : 426;
    sendmessage(cfd, num, ok ? "retr complete" : "connect close");
}

void ftpepollserver::STOR(int cfd, Client& client, std::string& path) {
    if (path.empty()) {
        sendmessage(cfd, 501, "need file name");
        return;
    }
    if (client.pasvfd == -1) {
        sendmessage(cfd, 425, "need pasv first");
        return;
    }

    int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        sendmessage(cfd, 550, "create file failed");
        return;
    }

    sendmessage(cfd, 150, "open data connect for stor");
    int datafd = acceptdata(client);
    if (datafd == -1) {
        close(fd);
        sendmessage(cfd, 425, "data connect failed");
        return;
    }

    char buf[4096];
    ssize_t n = 0;
    bool ok = true;
    while ((n = recv(datafd, buf, sizeof(buf), 0)) > 0) {
        if (write(fd, buf, static_cast<size_t>(n)) != n) {
            ok = false;
            break;
        }
    }

    close(fd);
    close(datafd);
    int num = ok ? 226 : 426;
    sendmessage(cfd, num, ok ? "stor complete" : "connect close");
}

void ftpepollserver::closePASV(Client& client) {
    if (client.pasvfd != -1) {
        close(client.pasvfd);
        client.pasvfd = -1;
    }
}

std::string getlineok(std::string line) {
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
        line.pop_back();
    }
    return line;
}

std::string readline(int fd) {
    std::string line;
    char ch = 0;
    while (true) {
        ssize_t n = recv(fd, &ch, 1, 0);
        line.push_back(ch);
        if (ch == '\n') {
            break;
        }
    }
    return getlineok(line);
}

bool sendn(int fd, char* data, size_t len) {
    while (len > 0) {
        ssize_t n = send(fd, data, len, 0);
        if (n <= 0) {
            return false;
        }
        data += n;
        len -= static_cast<size_t>(n);
    }
    return true;
}

bool sendstr(int fd, std::string& text) {
    return sendn(fd, text.data(), text.size());
}

void sendmessage(int fd, int code, std::string text) {
    std::string line = std::to_string(code) + " " + text + "\r\n";
    sendstr(fd, line);
}

void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
int main() {
    ftpepollserver server(2100);
    server.run();
    return 0;
}
