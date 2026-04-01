#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
class Socket {
   private:
    int fd_ = -1;

   public:
    Socket(int domain, int type, int protocol) {
        fd_ = ::socket(domain, type, protocol);
    }
    ~Socket() {
        if (fd_ != -1) {
            ::close(fd_);
            fd_ = -1;
        }
    }
    int fd() { return fd_; }
    bool isValid() { return fd_ != -1; }
    bool enableReuseAddr() {
        int opt = 1;
        if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ==
            -1) {
            std::cerr << strerror(errno) << std::endl;
            return false;
        }
        return true;
    }
};
class IPv4Address {
   private:
    sockaddr_in addr_;

   public:
    IPv4Address(uint16_t port) {
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        addr_.sin_addr.s_addr = INADDR_ANY;
    }
    sockaddr* change() { return reinterpret_cast<sockaddr*>(&addr_); }
};
class Server {
   private:
    Socket serversocket_;
    uint16_t port_;
    int epfd;
    void handle_client(int fd) {}

   public:
    Server(uint16_t port)
        : port_(port), serversocket_(AF_INET, SOCK_STREAM, 0) {
        serversocket_.enableReuseAddr();
        IPv4Address addr(port_);
        ::bind(serversocket_.fd(), addr.change(), sizeof(addr));
        ::listen(serversocket_.fd(), 100);
        epfd = ::epoll_create1(0);
        epoll_event events;
        events.events = EPOLLIN;
        events.data.fd = serversocket_.fd();
        ::epoll_ctl(epfd, EPOLL_CTL_ADD, serversocket_.fd(), &events);
        epoll_event EVENTS[100];
        while (1) {
            int n = epoll_wait(epfd, EVENTS, 99, -1);
            for (int i = 0; i < n; i++) {
                if (EVENTS[i].data.fd == serversocket_.fd()) {
                    int cfd = accept(serversocket_.fd(), NULL, NULL);
                    epoll_event events;
                    events.events = EPOLLET;
                    events.data.fd = cfd;
                    ::epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &events);
                } else {
                    handle_client(EVENTS[i].data.fd);
                }
            }
        }
    }
};
// 封装epoll
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <vector>
class Socket {
   private:
    int fd_ = -1;

   public:
    Socket(int domain, int type, int protocol) {
        fd_ = ::socket(domain, type, protocol);
    }
    ~Socket() {
        if (fd_ != -1) {
            ::close(fd_);
            fd_ = -1;
        }
    }
    int fd() { return fd_; }
    bool is_Valid() { return fd_ != -1; }
    bool enableReuseAddr() {
        int opt = 1;
        if (::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ==
            -1) {
            return false;
        }
        return true;
    }
};
class IPv4Address {
   private:
    sockaddr_in addr_;

   public:
    IPv4Address(uint16_t port) {
        memset(&addr_, 0, sizeof(addr_));
        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        addr_.sin_addr.s_addr = INADDR_ANY;
    }
    sockaddr* change() { return reinterpret_cast<sockaddr*>(&addr_); }
};
class EPOLL {
   private:
    int epfd_ = -1;

   public:
    EPOLL() { epfd_ = epoll_create1(0); }
    ~EPOLL() {
        if (epfd_ != -1) {
            ::close(epfd_);
            epfd_ = -1;
        }
    }
    void add(int fd, uint32_t events) {
        epoll_event ev;
        ev.data.fd = fd;
        ev.events = events;
        epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
    }
    int wait(std::vector<epoll_event>& events, int timeout) {
        int n = epoll_wait(epfd_, events.data(), 99, timeout);
        return n;
    }
};
void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
class EPOLLserver {
   private:
    EPOLL epoll;
    Socket serversocket_;
    uint16_t port_;

   public:
    EPOLLserver(uint16_t port)
        : port_(port), serversocket_(AF_INET, SOCK_STREAM, 0) {
        IPv4Address addr(port_);
        bind(serversocket_.fd(), addr.change(), sizeof(addr));
        listen(serversocket_.fd(), 10);
        epoll.add(serversocket_.fd(), EPOLLIN);
        std::vector<epoll_event> EVENTS(100);
        while (1) {
            int n = epoll.wait(EVENTS, -1);
            for (int i = 0; i < n; i++) {
                if (EVENTS[i].data.fd == serversocket_.fd()) {
                    int cfd = accept(serversocket_.fd(), NULL, NULL);
                    set_nonblock(cfd);
                    epoll.add(cfd, EPOLLIN);
                } else {
                    ssize_t n;
                    char buf[1024];
                    while (n = (recv(EVENTS[i].data.fd, buf, sizeof(buf), 0)) >
                               0) {
                        send(EVENTS[i].data.fd, buf, n, 0);
                    }
                }
            }
        }
    }
};
int main() {
    EPOLLserver server(8080);
    return 0;
}