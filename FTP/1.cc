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
struct Client{
    std::string readbuf;
    std::string writebuf;
};
class Threadpool{
    public:
     Threadpool(ssize_t num = std::thread::hardware_concurrency());
     ~Threadpool();
     void addtask(std::function<void()> task);
     private:
      void work();
      std::mutex mutex_;
      std::condition_variable cond_;
      std::vector<std::thread> threads_;
      std::queue<std::function<void()>> tasks_;
};
class ftpepollserver {
   public:
    ftpepollserver(uint16_t port);
    ~ftpepollserver();
    void run();

   private:
    void handle_read(int fd, int pos);
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

Threadpool::Threadpool(ssize_t num) {
    if (num == 0) {
        num = 4;
    }
    for (ssize_t i = 0; i < num; ++i) {
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
    epfd = epoll_create1(0);
    account["ftp"] = "123456";
}
ftpepollserver::~ftpepollserver() {
    close(epfd);
}