#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

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
    IPV4(const char* ip, uint16_t port) {
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &addr.sin_addr);
    }
    sockaddr* change() { return (sockaddr*)&addr; }
    socklen_t len() { return sizeof(addr); }
    uint16_t getport() { return ntohs(addr.sin_port); }
    std::string getip() {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
        return std::string(ip);
    }
};

class Ftpserver {
   private:
    uint16_t port_;
    Socket listensock_;
    std::string current_dir_;

    std::string readline(int fd);
    std::string getLocalIP();
    void sendmessage(int fd, std::string num, std::string s);
    int createPASVsocket(int& datafd);
    void handlePASV(int cfd, int& datafd);
    void handleLIST(int cfd, int datafd);
    void handleRETR(int cfd, int datafd, std::string filename);
    void handleSTOR(int cfd, int datafd, std::string filename);
    void handleCWD(int cfd, std::string dirname);
    void handlePWD(int cfd);
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
        if (c == '\n') {
            break;
        }
        if (c != '\r') {
            line += c;
        }
    }
    return line;
}

std::string Ftpserver::getLocalIP() {
    struct ifaddrs *ifap, *ifa;
    std::string ip = "127.0.0.1";

    if (getifaddrs(&ifap) == -1) {
        return ip;
    }

    for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;
        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in* sa = (struct sockaddr_in*)ifa->ifa_addr;
            char addr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &sa->sin_addr, addr, sizeof(addr));
            std::string addr_str(addr);
            if (addr_str != "127.0.0.1") {
                ip = addr_str;
                break;
            }
        }
    }
    freeifaddrs(ifap);
    return ip;
}

void Ftpserver::sendmessage(int fd, std::string num, std::string s) {
    std::string message = num + " " + s + "\r\n";
    write(fd, message.data(), message.size());
}

int Ftpserver::createPASVsocket(int& datafd) {
    datafd = socket(AF_INET, SOCK_STREAM, 0);
    if (datafd == -1)
        return -1;

    IPV4 addr(0);
    if (bind(datafd, addr.change(), addr.len()) == -1) {
        close(datafd);
        return -1;
    }

    if (listen(datafd, 1) == -1) {
        close(datafd);
        return -1;
    }

    socklen_t len = addr.len();
    if (getsockname(datafd, addr.change(), &len) == -1) {
        close(datafd);
        return -1;
    }

    return addr.getport();
}

void Ftpserver::handlePASV(int cfd, int& datafd) {
    int port = createPASVsocket(datafd);
    if (port == -1) {
        sendmessage(cfd, "425", "Cannot create data socket");
        return;
    }

    std::string ip = getLocalIP();
    // 将IP地址的点分十进制转换为逗号分隔的整数
    int h1, h2, h3, h4;
    sscanf(ip.c_str(), "%d.%d.%d.%d", &h1, &h2, &h3, &h4);
    int p1 = port / 256;
    int p2 = port % 256;

    std::string s = "Entering Passive Mode (" + std::to_string(h1) + "," +
                    std::to_string(h2) + "," + std::to_string(h3) + "," +
                    std::to_string(h4) + "," + std::to_string(p1) + "," +
                    std::to_string(p2) + ")";
    sendmessage(cfd, "227", s);
    std::cout << "PASV mode: IP=" << ip << ", port=" << port << std::endl;
}

void Ftpserver::handleLIST(int cfd, int datafd) {
    int dfd = accept(datafd, nullptr, nullptr);
    close(datafd);

    if (dfd == -1) {
        sendmessage(cfd, "425", "Cannot open data connection");
        return;
    }

    sendmessage(cfd, "150", "Opening ASCII mode data connection for file list");

    DIR* dir = opendir(current_dir_.empty() ? "." : current_dir_.c_str());
    if (dir == nullptr) {
        sendmessage(cfd, "550", "Cannot open directory");
        close(dfd);
        return;
    }

    struct dirent* ent;
    while ((ent = readdir(dir)) != nullptr) {
        if (ent->d_name[0] == '.')
            continue;  // 跳过隐藏文件

        std::string line = ent->d_name;
        struct stat st;
        std::string fullpath =
            (current_dir_.empty() ? "" : current_dir_ + "/") + ent->d_name;
        if (stat(fullpath.c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                line += "/";
            }
        }
        line += "\r\n";
        write(dfd, line.data(), line.size());
    }
    closedir(dir);
    close(dfd);
    sendmessage(cfd, "226", "Directory send OK");
}

void Ftpserver::handleRETR(int cfd, int datafd, std::string filename) {
    int dfd = accept(datafd, nullptr, nullptr);
    close(datafd);

    if (dfd == -1) {
        sendmessage(cfd, "425", "Cannot open data connection");
        return;
    }

    std::string fullpath =
        (current_dir_.empty() ? "" : current_dir_ + "/") + filename;
    int fd = open(fullpath.c_str(), O_RDONLY);
    if (fd == -1) {
        sendmessage(cfd, "550", "File not found");
        close(dfd);
        return;
    }

    sendmessage(cfd, "150",
                "Opening ASCII mode data connection for file transfer");

    char buf[8192];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        write(dfd, buf, n);
    }

    close(fd);
    close(dfd);
    sendmessage(cfd, "226", "Transfer complete");
}

void Ftpserver::handleSTOR(int cfd, int datafd, std::string filename) {
    int dfd = accept(datafd, nullptr, nullptr);
    close(datafd);

    if (dfd == -1) {
        sendmessage(cfd, "425", "Cannot open data connection");
        return;
    }

    std::string fullpath =
        (current_dir_.empty() ? "" : current_dir_ + "/") + filename;
    int fd = open(fullpath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        sendmessage(cfd, "550", "Cannot create file");
        close(dfd);
        return;
    }

    sendmessage(cfd, "150",
                "Opening ASCII mode data connection for file upload");

    char buf[8192];
    ssize_t n;
    while ((n = read(dfd, buf, sizeof(buf))) > 0) {
        write(fd, buf, n);
    }

    close(fd);
    close(dfd);
    sendmessage(cfd, "226", "Upload complete");
}

void Ftpserver::handleCWD(int cfd, std::string dirname) {
    std::string newdir;
    if (dirname == "..") {
        // 返回上级目录
        size_t pos = current_dir_.rfind('/');
        if (pos == std::string::npos) {
            newdir = "";
        } else {
            newdir = current_dir_.substr(0, pos);
        }
    } else if (dirname == ".") {
        newdir = current_dir_;
    } else if (dirname[0] == '/') {
        newdir = dirname;
    } else {
        newdir = current_dir_.empty() ? dirname : current_dir_ + "/" + dirname;
    }

    // 检查目录是否存在
    DIR* dir = opendir(newdir.empty() ? "." : newdir.c_str());
    if (dir == nullptr) {
        sendmessage(cfd, "550", "Directory not found");
        return;
    }
    closedir(dir);

    current_dir_ = newdir;
    sendmessage(
        cfd, "250",
        "Directory changed to " + (current_dir_.empty() ? "/" : current_dir_));
}

void Ftpserver::handlePWD(int cfd) {
    std::string pwd = current_dir_.empty() ? "/" : current_dir_;
    sendmessage(cfd, "257", "\"" + pwd + "\" is current directory");
}

void Ftpserver::handleclient(int cfd) {
    int datafd = -1;
    sendmessage(cfd, "220", "FTP server ready");

    while (1) {
        std::string cmd = readline(cfd);
        if (cmd.empty()) {
            break;
        }

        std::cout << "Received: " << cmd << std::endl;

        std::string cmd_name, cmd_arg;
        size_t space = cmd.find(' ');
        if (space == std::string::npos) {
            cmd_name = cmd;
            cmd_arg = "";
        } else {
            cmd_name = cmd.substr(0, space);
            cmd_arg = cmd.substr(space + 1);
        }

        // 转换为大写
        for (auto& c : cmd_name)
            c = toupper(c);

        if (cmd_name == "USER") {
            sendmessage(cfd, "331", "User name okay, need password");
        } else if (cmd_name == "PASS") {
            sendmessage(cfd, "230", "User logged in");
        } else if (cmd_name == "PASV") {
            handlePASV(cfd, datafd);
        } else if (cmd_name == "LIST") {
            if (datafd == -1) {
                sendmessage(cfd, "425", "Use PASV first");
            } else {
                handleLIST(cfd, datafd);
                datafd = -1;
            }
        } else if (cmd_name == "RETR") {
            if (datafd == -1) {
                sendmessage(cfd, "425", "Use PASV first");
            } else {
                handleRETR(cfd, datafd, cmd_arg);
                datafd = -1;
            }
        } else if (cmd_name == "STOR") {
            if (datafd == -1) {
                sendmessage(cfd, "425", "Use PASV first");
            } else {
                handleSTOR(cfd, datafd, cmd_arg);
                datafd = -1;
            }
        } else if (cmd_name == "CWD") {
            handleCWD(cfd, cmd_arg);
        } else if (cmd_name == "PWD") {
            handlePWD(cfd);
        } else if (cmd_name == "QUIT") {
            sendmessage(cfd, "221", "Goodbye");
            break;
        } else {
            sendmessage(cfd, "502", "Command not implemented");
        }
    }
    close(cfd);
}

Ftpserver::Ftpserver(uint16_t port)
    : port_(port), listensock_(AF_INET, SOCK_STREAM, 0) {
    listensock_.enableport();
    IPV4 addr(port_);

    if (bind(listensock_.fd(), addr.change(), addr.len()) == -1) {
        std::cerr << "Bind failed: " << strerror(errno) << std::endl;
        exit(1);
    }

    if (listen(listensock_.fd(), 10) == -1) {
        std::cerr << "Listen failed: " << strerror(errno) << std::endl;
        exit(1);
    }

    std::cout << "FTP Server started on port " << port_ << std::endl;
    std::cout << "Local IP: " << getLocalIP() << std::endl;

    while (1) {
        int cfd = accept(listensock_.fd(), nullptr, nullptr);
        if (cfd == -1) {
            std::cerr << "Accept failed: " << strerror(errno) << std::endl;
            continue;
        }

        std::cout << "New client connected" << std::endl;
        std::thread t(&Ftpserver::handleclient, this, cfd);
        t.detach();
    }
}

int main() {
    Ftpserver server(2100);
    return 0;
}