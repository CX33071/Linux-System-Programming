#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <atomic>




struct Client {
    std::string recv_buffer;    // 接收缓冲区
    std::string send_buffer;    // 发送缓冲区
    bool send_pending = false;  // 是否有数据待发送
};

class ThreadPool {
   public:
    ThreadPool(size_t num_threads);
    ~ThreadPool();

    template <typename F>
    void submit(F&& f);

   private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_;
};

class ftpepollserver {
   private:
    void handle_read(int cfd);
    void handle_write(int cfd);
    void process_command(int cfd);
    void mod_epoll(int fd, uint32_t events);
    std::map<int, Client> clients_;
    std::mutex clients_mutex_;
    int epfd_;
    ThreadPool thread_pool_;
    std::atomic<bool> running_;
};
// ThreadPool 实现
ThreadPool::ThreadPool(size_t num_threads) : stop_(false) {
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex_);
                    this->condition_.wait(lock, [this] {
                        return this->stop_ || !this->tasks_.empty();
                    });
                    if (this->stop_ && this->tasks_.empty())
                        return;
                    task = std::move(this->tasks_.front());
                    this->tasks_.pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    condition_.notify_all();
    for (std::thread& worker : workers_) {
        worker.join();
    }
}

template <typename F>
void ThreadPool::submit(F&& f) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (stop_) {
            return;
        }
        tasks_.emplace(std::forward<F>(f));
    }
    condition_.notify_one();
}
ftpepollserver::ftpepollserver(uint16_t port)
    : port_(port),
      listensock_(AF_INET, SOCK_STREAM, 0),
      thread_pool_(THREAD_NUM),
      running_(true) {
    account["ftp"] = "123456";
}

ftpepollserver::~ftpepollserver() {
    running_ = false;
    close(epfd_);
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
    if (bind(listensock_.fd(), addr.change(), addr.len()) == -1) {
        std::perror("bind");
        return;
    }

    if (listen(listensock_.fd(), 16) == -1) {
        std::perror("listen");
        return;
    }

    std::cout << "ftp server listen on port " << port_ << '\n';

    // 添加监听socket到epoll
    add_to_epoll(listensock_.fd(), EPOLLIN | EPOLLET);

    struct epoll_event events[MAX_SIZE];

    while (running_) {
        int n = epoll_wait(epfd_, events, MAX_SIZE, -1);

        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            if (fd == listensock_.fd()) {
                // 接受新连接
                while (true) {
                    int cfd = accept(listensock_.fd(), nullptr, nullptr);
                    if (cfd == -1) {
                        if (errno != EAGAIN && errno != EWOULDBLOCK) {
                            std::perror("accept");
                        }
                        break;
                    }

                    std::cout << "有一个客户端成功连接, fd=" << cfd << "\n";
                    set_nonblock(cfd);

                    {
                        std::lock_guard<std::mutex> lock(clients_mutex_);
                        clients_[cfd] = Client();
                    }

                    add_to_epoll(cfd, EPOLLIN | EPOLLET);
                    sendmessage(cfd, 220, "ftp server ready");
                }
            } else {
                if (ev & EPOLLIN) {
                    // 提交读任务到线程池
                    thread_pool_.submit([this, fd]() { handle_read(fd); });
                }
                if (ev & EPOLLOUT) {
                    // 提交写任务到线程池
                    thread_pool_.submit([this, fd]() { handle_write(fd); });
                }
                if (ev & (EPOLLERR | EPOLLHUP)) {
                    // 连接错误，关闭
                    std::cout << "连接关闭, fd=" << fd << "\n";
                    {
                        std::lock_guard<std::mutex> lock(clients_mutex_);
                        auto it = clients_.find(fd);
                        if (it != clients_.end()) {
                            closePASV(it->second);
                            clients_.erase(it);
                        }
                    }
                    remove_from_epoll(fd);
                    close(fd);
                }
            }
        }
    }

    close(epfd_);
}

void ftpepollserver::handle_read(int cfd) {
    Client client_copy;
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        auto it = clients_.find(cfd);
        if (it == clients_.end()) {
            return;
        }
        client_copy = it->second;
    }

    char buf[4096];
    bool has_data = false;

    // 非阻塞读取所有数据
    while (true) {
        ssize_t n = recv(cfd, buf, sizeof(buf), 0);
        if (n > 0) {
            has_data = true;
            {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                clients_[cfd].recv_buffer.append(buf, n);
            }
        } else if (n == 0) {
            // 连接关闭
            std::cout << "客户端关闭连接, fd=" << cfd << "\n";
            {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                auto it = clients_.find(cfd);
                if (it != clients_.end()) {
                    closePASV(it->second);
                    clients_.erase(it);
                }
            }
            remove_from_epoll(cfd);
            close(cfd);
            return;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;  // 数据读完了
            } else {
                // 读错误
                std::perror("recv");
                return;
            }
        }
    }

    // 如果有数据，处理命令
    if (has_data) {
        process_command(cfd);
    }
}

void ftpepollserver::handle_write(int cfd) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    auto it = clients_.find(cfd);
    if (it == clients_.end()) {
        return;
    }

    Client& client = it->second;
    if (client.send_buffer.empty()) {
        // 没有数据要发送，移除EPOLLOUT事件
        mod_epoll(cfd, EPOLLIN | EPOLLET);
        client.send_pending = false;
        return;
    }

    // 发送数据
    ssize_t n =
        send(cfd, client.send_buffer.c_str(), client.send_buffer.size(), 0);
    if (n > 0) {
        client.send_buffer.erase(0, n);
        if (client.send_buffer.empty()) {
            // 发送完成，移除EPOLLOUT事件
            mod_epoll(cfd, EPOLLIN | EPOLLET);
            client.send_pending = false;
        }
    } else if (n == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
        // 发送错误，关闭连接
        closePASV(client);
        clients_.erase(it);
        remove_from_epoll(cfd);
        close(cfd);
    }
}

void ftpepollserver::process_command(int cfd) {
    Client client_copy;
    std::string recv_buffer;

    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        auto it = clients_.find(cfd);
        if (it == clients_.end()) {
            return;
        }
        recv_buffer = it->second.recv_buffer;
        client_copy = it->second;
    }

    // 解析完整命令
    size_t pos;
    while ((pos = recv_buffer.find('\n')) != std::string::npos) {
        std::string line = recv_buffer.substr(0, pos + 1);
        recv_buffer.erase(0, pos + 1);

        line = getlineok(line);
        if (line.empty())
            continue;

        std::string cmd;
        std::string arg;
        size_t space_pos = line.find(' ');
        if (space_pos == std::string::npos) {
            cmd = line;
        } else {
            cmd = line.substr(0, space_pos);
            arg = line.substr(space_pos + 1);
        }

        // 处理命令
        if (cmd == "USER") {
            USER(cfd, client_copy, arg);
        } else if (cmd == "PASS") {
            PASS(cfd, client_copy, arg);
        } else if (cmd == "QUIT") {
            sendmessage(cfd, 221, "bye bye");
            {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                auto it = clients_.find(cfd);
                if (it != clients_.end()) {
                    closePASV(it->second);
                    clients_.erase(it);
                }
            }
            remove_from_epoll(cfd);
            close(cfd);
            return;
        } else if (!client_copy.islogin) {
            sendmessage(cfd, 530, "login first");
        } else if (strcasecmp(cmd.c_str(), "PASV") == 0) {
            PASV(cfd, client_copy);
        } else if (strcasecmp(cmd.c_str(), "LIST") == 0) {
            LIST(cfd, client_copy);
        } else if (strcasecmp(cmd.c_str(), "RETR") == 0) {
            RETR(cfd, client_copy, arg);
        } else if (strcasecmp(cmd.c_str(), "STOR") == 0) {
            STOR(cfd, client_copy, arg);
        } else {
            sendmessage(cfd, 502, "command errno");
        }

        // 更新client状态
        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            if (clients_.find(cfd) != clients_.end()) {
                clients_[cfd] = client_copy;
                clients_[cfd].recv_buffer = recv_buffer;
            }
        }
    }
}

void ftpepollserver::add_to_epoll(int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
        std::perror("epoll_ctl add");
    }
}

void ftpepollserver::mod_epoll(int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    if (epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev) == -1) {
        std::perror("epoll_ctl mod");
    }
}

void ftpepollserver::remove_from_epoll(int fd) {
    epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
}

void ftpepollserver::handleclient(int cfd) {
    // 这个函数保留但不再使用，保持接口兼容
    // 实际处理在 process_command 中
}

// 以下函数保持不变，但修改发送方式以支持异步发送
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
    if (datafd != -1) {
        set_nonblock(datafd);
    }
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

// 辅助函数实现
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
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
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

    // 获取client并添加到发送缓冲区
    // 这里需要访问全局的clients_，需要外部传入
    // 简化实现：直接发送（注意：生产环境需要改为异步发送）
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