#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
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
    void handle_read(int fd);
    void handle_write(int fd);
    void add_epoll(int fd, uint32_t events);
    void del_epoll(int fd, uint32_t events);
    void mod_epoll(int fd, uint32_t events);
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
    std::map<int, Client> clients;
    std::map<std::string, std::string> account;
    std::mutex clients_mutex_;  // 添加mutex保护clients
};

std::string getlineok(std::string line);
bool sendn(int fd, char* data, size_t len);
bool sendstr(int fd, std::string& text);
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

Threadpool::Threadpool(size_t num) {
    if (num == 0) {
        num = 4;
    }
    for (size_t i = 0; i < num; ++i) {
        threads_.emplace_back(&Threadpool::work, this);
    }
}

Threadpool::~Threadpool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }
    cond_.notify_all();
    for (auto& t : threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void Threadpool::addtask(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push(std::move(task));
    }
    cond_.notify_one();
}

void Threadpool::work() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });
            if (stop_ && tasks_.empty()) {
                return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        task();
    }
}

ftpepollserver::ftpepollserver(uint16_t port)
    : port_(port), listensock_(AF_INET, SOCK_STREAM, 0) {
    account["ftp"] = "123456";
    set_nonblock(listensock_.fd());
    epfd = epoll_create1(0);
}

ftpepollserver::~ftpepollserver() {
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
    std::cout << "ftp server listen on " << port_ << '\n';
    add_epoll(listensock_.fd(), EPOLLET | EPOLLIN);

    struct epoll_event events[MAX_SIZE];
    while (1) {
        int n = epoll_wait(epfd, events, MAX_SIZE, -1);
        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;
            if (fd == listensock_.fd()) {
                while (1) {
                    int cfd = accept(listensock_.fd(), nullptr, nullptr);
                    if (cfd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }
                        break;
                    }
                    std::cout << "有一个客户端成功连接\n";
                    set_nonblock(cfd);
                    {
                        std::lock_guard<std::mutex> lock(clients_mutex_);
                        clients[cfd] = Client();
                        clients[cfd].writebuf += "220 ftp server ready\r\n";
                    }
                    add_epoll(cfd, EPOLLIN | EPOLLET | EPOLLOUT | EPOLLRDHUP);
                }
            } else if (events[i].events & EPOLLIN) {
                // ✅ 主线程直接处理读，不丢线程池
                handle_read(fd);
            } else if (events[i].events & EPOLLOUT) {
                // ✅ 主线程直接处理写，不丢线程池
                handle_write(fd);
            }
        }
    }
}

void ftpepollserver::handle_read(int fd) {
    char buf[4096];

    // 获取client的引用（主线程独占，安全）
    std::lock_guard<std::mutex> lock(clients_mutex_);
    auto it = clients.find(fd);
    if (it == clients.end()) {
        return;
    }
    Client& client = it->second;

    // 非阻塞读取所有数据
    while (1) {
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0) {
            if (n == 0 || (n == -1 && errno != EAGAIN)) {
                // 连接关闭或错误
                close(fd);
                clients.erase(fd);
                del_epoll(fd, 0);
                return;
            }
            break;  // EAGAIN，数据读完了
        }
        client.readbuf.append(buf, n);
    }

    // 解析完整命令
    while (1) {
        size_t pos = client.readbuf.find("\n");
        if (pos == std::string::npos) {
            break;
        }
        std::string line = client.readbuf.substr(0, pos + 1);
        client.readbuf.erase(0, pos + 1);
        line = getlineok(line);

        std::string cmd, arg;
        size_t p = line.find(' ');
        if (p == std::string::npos) {
            cmd = line;
        } else {
            cmd = line.substr(0, p);
            arg = line.substr(p + 1);
        }

        if (cmd == "USER") {
            USER(fd, client, arg);
        } else if (cmd == "PASS") {
            PASS(fd, client, arg);
        } else if (cmd == "QUIT") {
            client.writebuf += "221 bye bye\r\n";
            // QUIT后关闭连接
            close(fd);
            clients.erase(fd);
            del_epoll(fd, 0);
            return;
        } else if (!client.islogin) {
            client.writebuf += "530 login first\r\n";
        } else if (strcasecmp(cmd.c_str(), "PASV") == 0) {
            PASV(fd, client);
        } else if (strcasecmp(cmd.c_str(), "LIST") == 0) {
            // ✅ LIST丢线程池
            int cfd = fd;
            pool.addtask([this, cfd]() {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                auto it = clients.find(cfd);
                if (it == clients.end())
                    return;
                LIST(cfd, it->second);
            });
        } else if (strcasecmp(cmd.c_str(), "RETR") == 0) {
            // ✅ RETR丢线程池，需要拷贝arg
            int cfd = fd;
            std::string path = arg;
            pool.addtask([this, cfd, path]() {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                auto it = clients.find(cfd);
                if (it == clients.end())
                    return;
                RETR(cfd, it->second, const_cast<std::string&>(path));
            });
        } else if (strcasecmp(cmd.c_str(), "STOR") == 0) {
            // ✅ STOR丢线程池，需要拷贝arg
            int cfd = fd;
            std::string path = arg;
            pool.addtask([this, cfd, path]() {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                auto it = clients.find(cfd);
                if (it == clients.end())
                    return;
                STOR(cfd, it->second, const_cast<std::string&>(path));
            });
        } else {
            client.writebuf += "502 command error\r\n";
        }
    }

    // 如果有数据要发送，确保EPOLLOUT事件已注册
    if (!client.writebuf.empty()) {
        mod_epoll(fd, EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP);
    }
}

void ftpepollserver::handle_write(int fd) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    auto it = clients.find(fd);
    if (it == clients.end()) {
        return;
    }
    Client& client = it->second;

    while (!client.writebuf.empty()) {
        ssize_t n = send(fd, client.writebuf.data(), client.writebuf.size(), 0);
        if (n <= 0) {
            if (n == -1 && errno == EAGAIN) {
                break;  // 缓冲区满，下次再发
            }
            // 发送错误，关闭连接
            close(fd);
            clients.erase(fd);
            del_epoll(fd, 0);
            return;
        } else {
            client.writebuf.erase(0, n);
        }
    }

    // 发送完成后，移除EPOLLOUT事件
    if (client.writebuf.empty()) {
        mod_epoll(fd, EPOLLIN | EPOLLET | EPOLLRDHUP);
    }
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

void ftpepollserver::mod_epoll(int fd, uint32_t events) {
    epoll_event event{};
    event.events = events;
    event.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event);
}

// 修改USER，使用writebuf而不是sendmessage
void ftpepollserver::USER(int cfd, Client& client, std::string& user) {
    if (user.empty()) {
        client.writebuf += "501 username\r\n";
        return;
    }
    if (account.count(user) == 0) {
        client.writebuf += "530 username not exist\r\n";
        return;
    }
    client.username = user;
    client.hasuser = true;
    client.islogin = false;
    client.writebuf += "331 need password\r\n";
}

// 修改PASS，使用writebuf而不是sendmessage
void ftpepollserver::PASS(int cfd, Client& client, std::string& pass) {
    if (!client.hasuser) {
        client.writebuf += "503 need username\r\n";
        return;
    }
    auto it = account.find(client.username);
    if (it == account.end() || it->second != pass) {
        client.writebuf += "530 password error\r\n";
        return;
    }
    client.islogin = true;
    client.writebuf += "230 login successful\r\n";
}

// 修改PASV，使用writebuf而不是sendmessage
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
    client.writebuf += "227 Entering Passive Mode (" + std::to_string(ip[0]) +
                       "," + std::to_string(ip[1]) + "," +
                       std::to_string(ip[2]) + "," + std::to_string(ip[3]) +
                       "," + std::to_string(p1) + "," + std::to_string(p2) +
                       ")\r\n";
}

int ftpepollserver::acceptdata(Client& client) {
    if (client.pasvfd == -1) {
        return -1;
    }
    int datafd = accept(client.pasvfd, nullptr, nullptr);
    closePASV(client);
    return datafd;
}

// 修改LIST，使用writebuf而不是sendmessage
void ftpepollserver::LIST(int cfd, Client& client) {
    if (client.pasvfd == -1) {
        client.writebuf += "425 need pasv first\r\n";
        return;
    }

    // 发送150响应到控制连接
    client.writebuf += "150 open data connection for list\r\n";

    int datafd = acceptdata(client);
    if (datafd == -1) {
        client.writebuf += "425 data connect failed\r\n";
        return;
    }

    DIR* dir = opendir(".");
    if (dir == nullptr) {
        close(datafd);
        client.writebuf += "550 open dir failed\r\n";
        return;
    }

    dirent* ent = nullptr;
    while ((ent = readdir(dir)) != nullptr) {
        std::string line = std::string(ent->d_name) + "\r\n";
        if (!sendn(datafd, line.data(), line.size())) {
            break;
        }
    }
    closedir(dir);
    close(datafd);
    client.writebuf += "226 list complete\r\n";
}

// 修改RETR，使用writebuf而不是sendmessage
void ftpepollserver::RETR(int cfd, Client& client, std::string& path) {
    if (path.empty()) {
        client.writebuf += "501 need file name\r\n";
        return;
    }
    if (client.pasvfd == -1) {
        client.writebuf += "425 need pasv first\r\n";
        return;
    }

    int fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        client.writebuf += "550 file not found\r\n";
        return;
    }

    client.writebuf += "150 open data connect for retr\r\n";

    int datafd = acceptdata(client);
    if (datafd == -1) {
        close(fd);
        client.writebuf += "425 data connect failed\r\n";
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
    if (ok) {
        client.writebuf += "226 retr complete\r\n";
    } else {
        client.writebuf += "426 connect close\r\n";
    }
}

// 修改STOR，使用writebuf而不是sendmessage
void ftpepollserver::STOR(int cfd, Client& client, std::string& path) {
    if (path.empty()) {
        client.writebuf += "501 need file name\r\n";
        return;
    }
    if (client.pasvfd == -1) {
        client.writebuf += "425 need pasv first\r\n";
        return;
    }

    int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        client.writebuf += "550 create file failed\r\n";
        return;
    }

    client.writebuf += "150 open data connect for stor\r\n";

    int datafd = acceptdata(client);
    if (datafd == -1) {
        close(fd);
        client.writebuf += "425 data connect failed\r\n";
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
    if (ok) {
        client.writebuf += "226 stor complete\r\n";
    } else {
        client.writebuf += "426 connect close\r\n";
    }
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

void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void set_block(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

int main() {
    ftpepollserver server(2100);
    server.run();
    return 0;
}