#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <condition_variable>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>
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
    std::string readbuf;
    std::string writebuf;
};
class Threadpool {
public:
    Threadpool(size_t num = std::thread::hardware_concurrency());
    ~Threadpool();
    void addtask(std::function<void()> task);

private:
    void work();

    bool stop_ = false;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::queue<std::function<void()>> tasks_;
    std::vector<std::thread> threads_;
};
class ftpepollserver {
   public:
    ftpepollserver(uint16_t port);
    ~ftpepollserver();
    void run();

   private:
    void handle_read(int fd,int pos);
    void handle_write(int fd);
    void add_epoll(int fd, uint32_t events);
    void del_epoll(int fd, uint32_t events);
    void mod_epoll(int fd, uint32_t events);
    void USER(int cfd, Client& client, std::string& user);
    void PASS(int cfd, Client& client, std::string& pass);
    void PASV(int cfd, Client& client);
    int acceptdata(Client& client);
    void LIST(int cfd, Client& client);
    void RETR(int cfd, Client& client, std::string path);
    void STOR(int cfd, Client& client, std::string path);
    void closePASV(Client& client);

    uint16_t port_;
    Socket listensock_;
    int epfd;
    Threadpool pool;
    std::map<int, Client> clients;
    std::map<std::string, std::string> account;
};

std::string getlineok(std::string line);
bool sendn(int fd, char* data, size_t len);
bool sendstr(int fd, std::string& text);
void sendmessage(int fd, int code, std::string text);
void set_nonblock(int fd);
void set_block(int fd);
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

Threadpool::Threadpool(size_t n){
    if(n==0){
        n = 4;
    }
    for (int i = 0; i < n;i++){
        std::thread t(&Threadpool::work, this);
        threads_.push_back(std::move(t));
    }
}
Threadpool::~Threadpool(){
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.notify_all();
    for (auto& t : threads_) {
        if(t.joinable()){
            t.join();
        }
    }
}
void Threadpool::work(){

}
void Threadpool::addtask(std::function<void()> task) {

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
        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;
            if (fd == listensock_.fd()) {
                while (1) {
                    int cfd = accept(listensock_.fd(), nullptr, nullptr);
                    std::cout << "有一个客户端成功连接\n";
                    set_nonblock(cfd);
                    clients[cfd] = Client();
                    clients[cfd].writebuf += "220 ftp server ready\r\n";
                    add_epoll(cfd, EPOLLIN | EPOLLET |EPOLLOUT);
                }
            } else if(events[i].events&EPOLLIN){
                size_t pos = 0;
                handle_read(fd,pos);
            } else if (events[i].events & EPOLLOUT) {
                handle_write(fd);
            }
            }
    }
}
void ftpepollserver::handle_read(int fd,int pos){
    size_t n;
    char buf[4096];
    while(1){
    n = read(fd, buf, sizeof(buf));
        if(n<=0){
            close(fd);
            return;
        }else{
    clients[fd].readbuf.append(buf,pos,n);
    pos += n;
        }
        std::string cmd, arg;
        int p = clients[fd].readbuf.find(' ');
        if(p==std::string::npos){
            cmd = clients[fd].readbuf;
        }else{
            cmd = clients[fd].readbuf.substr(0, p);
            arg = clients[fd].readbuf.substr(p + 1);
        }
        if(cmd=="USER"){
            USER(fd, clients[fd], arg);
        } else if (cmd == "PASS") {
            PASS(fd, clients[fd], arg);
        } else if (cmd == "QUIT") {
            clients[fd].writebuf += "221,bye bye\r\n";
        } else if (!strcasecmp(cmd.c_str(), "pasv")) {
        } else if (!strcasecmp(cmd.c_str(), "list")) {
            pool.addtask([this, fd]() { LIST(fd, clients[fd]); });
        } else if (!strcasecmp(cmd.c_str(), "stor")) {
            pool.addtask([this, fd,arg]() { STOR(fd, clients[fd],arg); });
        } else if (!strcasecmp(cmd.c_str(), "retr")) {
            pool.addtask([this, fd,arg]() { RETR(fd, clients[fd], arg); });
        }else{
            clients[fd].writebuf += "502 command errno\r\n";
        }
    }
}
void ftpepollserver::handle_write(int fd){
    size_t n;
    while(!clients[fd].writebuf.empty()){
        n = send(fd, clients[fd].writebuf.data(), clients[fd].writebuf.size(),0);
        clients[fd].writebuf.erase(0,n);
    }
    mod_epoll(fd, EPOLLIN | EPOLLET);
}
void ftpepollserver::add_epoll(int fd, uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
}

void ftpepollserver::del_epoll(int fd, uint32_t events) {
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &ev);
}
void ftpepollserver::mod_epoll(int fd,uint32_t events){
    epoll_event event{};
    event.events = events;
    event.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event);
}
void ftpepollserver::USER(int cfd, Client& client, std::string& user) {
    if (user.empty()) {
        clients[cfd].writebuf += "501 need username\r\n";
        return;
    }
    if (account.count(user) == 0) {
        clients[cfd].writebuf += "530 username exist\r\n";
        return;
    }
    client.username = user;
    client.hasuser = true;
    client.islogin = false;
    clients[cfd].writebuf += "331 need password\r\n";
}

void ftpepollserver::PASS(int cfd, Client& client, std::string& pass) {
    if (!client.hasuser) {
        clients[cfd].writebuf += "503 need username\r\n";
        return;
    }
    auto it = account.find(client.username);
    if (it == account.end() || it->second != pass) {
        clients[cfd].writebuf += "530 password errno\r\n";
        return;
    }
    client.islogin = true;
    clients[cfd].writebuf += "230 login succeful\r\n";
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
    std::string s = "";
    s = "227 Entering Passive Mode (" + std::to_string(ip[0]) + "," +
         std::to_string(ip[1]) + "," + std::to_string(ip[2]) + "," +
         std::to_string(ip[3]) + "," + std::to_string(p1) + "," +
         std::to_string(p2) + ")";
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

void ftpepollserver::RETR(int cfd, Client& client, std::string path) {
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

void ftpepollserver::STOR(int cfd, Client& client, std::string path) {
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

void set_block(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags |O_NONBLOCK);
}
int main() {
    ftpepollserver server(2100);
    server.run();
    return 0;
}
