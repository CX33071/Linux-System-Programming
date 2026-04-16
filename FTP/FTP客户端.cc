#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <cerrno>
#include <cstring>

class Socket {
   public:
    Socket(int domain, int type, int protocol);
    ~Socket();
    int fd() ;
    bool isvalid() ;
    bool enableport();

   private:
    int fd_ = -1;
};

class FTPClient {
   public:
    FTPClient(uint16_t port, std::string ip);
    void start();

   private:
    bool login();
    void LIST();
    void RETR( std::string args);
    void STOR( std::string args);
    int PASV();
    void sendmessage( std::string cmd);
    int getcode( std::string line) ;
    bool prasePASV( std::string resp, std::string& ip, int& port) ;
    std::string getfilename( std::string& path) ;
    void getcmd( std::string& input, std::string& first, std::string& second) ;

    Socket Client_;
};

std::string getlineok(std::string line);
std::string readline(int fd);
bool sendn(int fd,  char* s, size_t len);

Socket::Socket(int domain, int type, int protocol) : fd_(socket(domain, type, protocol)) {}

Socket::~Socket() {
    if (fd_ != -1) {
        close(fd_);
        fd_ = -1;
    }
}

int Socket::fd()  {
    return fd_;
}

bool Socket::isvalid()  {
    return fd_ != -1;
}

bool Socket::enableport()  {
    int opt = 1;
    if (setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cout << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

FTPClient::FTPClient(uint16_t port,  std::string ip) : Client_(AF_INET, SOCK_STREAM, 0) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    connect(Client_.fd(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    std::cout << readline(Client_.fd()) << '\n';
}

void FTPClient::start() {
    if (!login()) {
        return;
    }

    std::cout << "可用命令:PASV | LIST | RETR | STOR | QUIT\n";
    std::string line;
    while (true) {
        std::cout << "ftp: " << std::flush;
        if (!std::getline(std::cin, line)) {
            break;
        }
        line = getlineok(line);
        if (line.empty()) {
            continue;
        }

        std::string cmd;
        std::string rest;
         size_t pos = line.find(' ');
        if (pos == std::string::npos) {
            cmd = line;
        } else {
            cmd = line.substr(0, pos);
            rest = line.substr(pos + 1);
        }
        for (char& ch : cmd) {
            ch = static_cast<char>(::toupper(static_cast<unsigned char>(ch)));
        }

        if (cmd == "PASV") {
            int datafd = PASV();
            if (datafd != -1) {
                close(datafd);
            }
        } else if (cmd == "LIST") {
            LIST();
        } else if (cmd == "RETR") {
            RETR(rest);
        } else if (cmd == "STOR") {
            STOR(rest);
        } else if (cmd == "QUIT") {
            sendmessage("QUIT");
            std::cout << readline(Client_.fd()) << '\n';
            break;
        } else {
            std::cout << "不支持的命令\n";
        }
    }
}

bool FTPClient::login() {
    std::string user;
    std::string pass;

    std::cout << "请输入用户名: " << std::flush;
    if (!std::getline(std::cin, user)) {
        return false;
    }
    sendmessage("USER " + user);
    std::string resp = readline(Client_.fd());
    std::cout << resp << '\n';
    if (getcode(resp) != 331) {
        return false;
    }

    std::cout << "请输入密码: " << std::flush;
    if (!std::getline(std::cin, pass)) {
        return false;
    }
    sendmessage("PASS " + pass);
    resp = readline(Client_.fd());
    std::cout << resp << '\n';
    return getcode(resp) == 230;
}

void FTPClient::LIST() {
    int datafd = PASV();
    sendmessage("LIST");
    std::string resp = readline(Client_.fd());
    std::cout << resp << '\n';
    if (getcode(resp) != 150) {
        close(datafd);
        return;
    }

    char buf[4096];
    ssize_t n = 0;
    while ((n = recv(datafd, buf, sizeof(buf), 0)) > 0) {
        std::cout.write(buf, n);
    }
    close(datafd);

    resp = readline(Client_.fd());
    std::cout << resp << '\n';
}

void FTPClient::RETR( std::string args) {
    std::string remote;
    std::string local;
    getcmd(args, remote, local);
    if (remote.empty()) {
        std::cout << "用法: RETR 远程文件 本地文件\n";
        return;
    }
    if (local.empty()) {
        local = getfilename(remote);
    }

    int fd = open(local.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    int datafd = PASV();
    if (datafd == -1) {
        close(fd);
        return;
    }

    sendmessage("RETR " + remote);
    std::string resp = readline(Client_.fd());
    std::cout << resp << '\n';
    if (getcode(resp) != 150) {
        close(fd);
        close(datafd);
        return;
    }

    char buf[4096];
    ssize_t n = 0;
    while ((n = recv(datafd, buf, sizeof(buf), 0)) > 0) {
        write(fd, buf, static_cast<size_t>(n)) ;

    close(fd);
    close(datafd);
    resp = readline(Client_.fd());
    std::cout << resp << '\n';
    if (getcode(resp) == 226) {
        std::cout << "下载成功\n";
    }
}
}
void FTPClient::STOR( std::string args) {
    std::string local;
    std::string remote;
    getcmd(args, local, remote);
    if (local.empty()) {
        std::cout << "用法: STOR 本地文件 远程文件\n";
        return;
    }
    if (remote.empty()) {
        remote = getfilename(local);
    }

    int fd = open(local.c_str(), O_RDONLY);
    if (fd == -1) {
        std::perror("open");
        return;
    }

    int datafd = PASV();
    if (datafd == -1) {
        close(fd);
        return;
    }

    sendmessage("STOR " + remote);
    std::string resp = readline(Client_.fd());
    std::cout << resp << '\n';
    if (getcode(resp) != 150) {
        close(fd);
        close(datafd);
        return;
    }

    char buf[4096];
    ssize_t n = 0;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        if(!sendn(datafd, buf, static_cast<size_t>(n))){
            perror("sendn");
            return;
        };
    }

    close(fd);
    shutdown(datafd, SHUT_WR);
    close(datafd);
    resp = readline(Client_.fd());
    std::cout << resp << '\n';
    if (getcode(resp) == 226) {
        std::cout << "上传成功\n";
    }
}

int FTPClient::PASV() {
    sendmessage("PASV");
     std::string resp = readline(Client_.fd());
    std::cout << resp << '\n';
    if (getcode(resp) != 227) {
        return -1;
    }

    std::string ip;
    int port = 0;
    prasePASV(resp, ip, port);

    int datafd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    connect(datafd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    std::cout << "数据通道已经连接成功\n";
    return datafd;
}

void FTPClient::sendmessage( std::string cmd) {
     std::string line = cmd + "\r\n";
    if(!sendn(Client_.fd(), line.data(), line.size())){
        perror("sendn");
        return;
    };
}



int FTPClient::getcode( std::string line)  {
    if (line.size() < 3) {
        return -1;
    }
    return std::atoi(line.substr(0, 3).c_str());
}

bool FTPClient::prasePASV( std::string resp, std::string& ip, int& port)  {
     size_t left = resp.find('(');
     size_t right = resp.find(')');
    if (left == std::string::npos || right == std::string::npos || right <= left + 1) {
        return false;
    }

    int h1 = 0, h2 = 0, h3 = 0, h4 = 0, p1 = 0, p2 = 0;
    if (std::sscanf(resp.substr(left + 1, right - left - 1).c_str(), "%d,%d,%d,%d,%d,%d",
                    &h1, &h2, &h3, &h4, &p1, &p2) != 6) {
        return false;
    }

    ip = std::to_string(h1) + "." + std::to_string(h2) + "." + std::to_string(h3) + "." +
         std::to_string(h4);
    port = p1 * 256 + p2;
    return true;
}

std::string FTPClient::getfilename( std::string& path)  {
     size_t pos = path.find_last_of("/\\");
    return pos == std::string::npos ? path : path.substr(pos + 1);
}

void FTPClient::getcmd( std::string& input, std::string& first, std::string& second)  {
     size_t pos = input.find(' ');
    if (pos == std::string::npos) {
        first = getlineok(input);
        return;
    }
    first = getlineok(input.substr(0, pos));
    second = getlineok(input.substr(pos + 1));
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

bool sendn(int fd, char*data, size_t len) {
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


int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "serverip\n";
        return 1;
    }

    FTPClient client(2100, argv[1]);
    client.start();
    return 0;
}
