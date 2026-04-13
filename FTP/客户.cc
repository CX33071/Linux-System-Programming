#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>

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
};

class FTPClient {
   private:
    Socket control_sock_;
    int data_fd_;
    std::string server_ip_;
    bool is_logged_in_;

    void sendCommand(const std::string& cmd);
    std::string recvResponse();
    int getResponseCode(const std::string& response);
    void parsePASV(const std::string& response, std::string& ip, int& port);
    void connectDataChannel(const std::string& ip, int port);

   public:
    FTPClient(const std::string& server_ip, uint16_t port);
    ~FTPClient();

    bool login(const std::string& user, const std::string& pass);
    bool pasv();
    bool list();
    bool retr(const std::string& remote_file, const std::string& local_file);
    bool stor(const std::string& local_file, const std::string& remote_file);
    bool cwd(const std::string& dirname);
    bool pwd();
    bool quit();

    void start();
};

void FTPClient::sendCommand(const std::string& cmd) {
    std::string full_cmd = cmd + "\r\n";
    send(control_sock_.fd(), full_cmd.c_str(), full_cmd.size(), 0);
    std::cout << "> " << cmd << std::endl;
}

std::string FTPClient::recvResponse() {
    std::string response;
    char buf[4096];
    ssize_t n = recv(control_sock_.fd(), buf, sizeof(buf) - 1, 0);
    if (n > 0) {
        buf[n] = '\0';
        response = buf;
        std::cout << response;
    }
    return response;
}

int FTPClient::getResponseCode(const std::string& response) {
    if (response.length() >= 3) {
        return std::stoi(response.substr(0, 3));
    }
    return 0;
}

void FTPClient::parsePASV(const std::string& response,
                          std::string& ip,
                          int& port) {
    int h1, h2, h3, h4, p1, p2;
    sscanf(response.c_str(), "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
           &h1, &h2, &h3, &h4, &p1, &p2);
    ip = std::to_string(h1) + "." + std::to_string(h2) + "." +
         std::to_string(h3) + "." + std::to_string(h4);
    port = p1 * 256 + p2;
}

void FTPClient::connectDataChannel(const std::string& ip, int port) {
    data_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (data_fd_ == -1) {
        std::cerr << "Cannot create data socket" << std::endl;
        return;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    if (connect(data_fd_, (sockaddr*)&addr, sizeof(addr)) == -1) {
        std::cerr << "Cannot connect to data port: " << strerror(errno)
                  << std::endl;
        close(data_fd_);
        data_fd_ = -1;
    }
}

FTPClient::FTPClient(const std::string& server_ip, uint16_t port)
    : control_sock_(AF_INET, SOCK_STREAM, 0),
      data_fd_(-1),
      server_ip_(server_ip),
      is_logged_in_(false) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, server_ip.c_str(), &addr.sin_addr);

    if (connect(control_sock_.fd(), (sockaddr*)&addr, sizeof(addr)) == -1) {
        std::cerr << "Cannot connect to server: " << strerror(errno)
                  << std::endl;
        exit(1);
    }

    recvResponse();  // 接收欢迎信息
}

FTPClient::~FTPClient() {
    if (data_fd_ != -1) {
        close(data_fd_);
    }
}

bool FTPClient::login(const std::string& user, const std::string& pass) {
    sendCommand("USER " + user);
    std::string response = recvResponse();
    if (getResponseCode(response) != 331) {
        return false;
    }

    sendCommand("PASS " + pass);
    response = recvResponse();
    if (getResponseCode(response) == 230) {
        is_logged_in_ = true;
        return true;
    }
    return false;
}

bool FTPClient::pasv() {
    sendCommand("PASV");
    std::string response = recvResponse();
    if (getResponseCode(response) != 227) {
        return false;
    }

    std::string ip;
    int port;
    parsePASV(response, ip, port);
    connectDataChannel(ip, port);
    return data_fd_ != -1;
}

bool FTPClient::list() {
    if (!pasv()) {
        std::cerr << "Failed to enter passive mode" << std::endl;
        return false;
    }

    sendCommand("LIST");
    std::string response = recvResponse();
    if (getResponseCode(response) != 150) {
        close(data_fd_);
        data_fd_ = -1;
        return false;
    }

    // 接收数据
    char buf[8192];
    ssize_t n;
    while ((n = read(data_fd_, buf, sizeof(buf))) > 0) {
        write(1, buf, n);
    }

    close(data_fd_);
    data_fd_ = -1;

    response = recvResponse();
    return getResponseCode(response) == 226;
}

bool FTPClient::retr(const std::string& remote_file,
                     const std::string& local_file) {
    if (!pasv()) {
        std::cerr << "Failed to enter passive mode" << std::endl;
        return false;
    }

    sendCommand("RETR " + remote_file);
    std::string response = recvResponse();
    if (getResponseCode(response) != 150) {
        close(data_fd_);
        data_fd_ = -1;
        return false;
    }

    int fd = open(local_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        std::cerr << "Cannot create local file: " << strerror(errno)
                  << std::endl;
        close(data_fd_);
        data_fd_ = -1;
        return false;
    }

    char buf[8192];
    ssize_t n;
    while ((n = read(data_fd_, buf, sizeof(buf))) > 0) {
        write(fd, buf, n);
    }

    close(fd);
    close(data_fd_);
    data_fd_ = -1;

    response = recvResponse();
    return getResponseCode(response) == 226;
}

bool FTPClient::stor(const std::string& local_file,
                     const std::string& remote_file) {
    if (!pasv()) {
        std::cerr << "Failed to enter passive mode" << std::endl;
        return false;
    }

    sendCommand("STOR " + remote_file);
    std::string response = recvResponse();
    if (getResponseCode(response) != 150) {
        close(data_fd_);
        data_fd_ = -1;
        return false;
    }

    int fd = open(local_file.c_str(), O_RDONLY);
    if (fd == -1) {
        std::cerr << "Cannot open local file: " << strerror(errno) << std::endl;
        close(data_fd_);
        data_fd_ = -1;
        return false;
    }

    char buf[8192];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        write(data_fd_, buf, n);
    }

    close(fd);
    close(data_fd_);
    data_fd_ = -1;

    response = recvResponse();
    return getResponseCode(response) == 226;
}

bool FTPClient::cwd(const std::string& dirname) {
    sendCommand("CWD " + dirname);
    std::string response = recvResponse();
    return getResponseCode(response) == 250;
}

bool FTPClient::pwd() {
    sendCommand("PWD");
    std::string response = recvResponse();
    return getResponseCode(response) == 257;
}

bool FTPClient::quit() {
    sendCommand("QUIT");
    std::string response = recvResponse();
    return getResponseCode(response) == 221;
}

void FTPClient::start() {
    std::string user, pass;
    std::cout << "Username: ";
    std::getline(std::cin, user);
    std::cout << "Password: ";
    std::getline(std::cin, pass);

    if (!login(user, pass)) {
        std::cerr << "Login failed" << std::endl;
        return;
    }

    std::cout << "\nFTP Client ready. Commands: ls, get <file>, put <file>, cd "
                 "<dir>, pwd, quit\n"
              << std::endl;

    std::string line;
    while (true) {
        std::cout << "ftp> ";
        std::getline(std::cin, line);

        if (line.empty())
            continue;

        std::string cmd, arg;
        size_t space = line.find(' ');
        if (space == std::string::npos) {
            cmd = line;
            arg = "";
        } else {
            cmd = line.substr(0, space);
            arg = line.substr(space + 1);
        }

        if (cmd == "ls" || cmd == "LIST") {
            if (!list()) {
                std::cerr << "List failed" << std::endl;
            }
        } else if (cmd == "get" || cmd == "RETR") {
            if (arg.empty()) {
                std::cout << "Usage: get <remote_file> [local_file]"
                          << std::endl;
                continue;
            }
            std::string local_file = arg;
            size_t slash = arg.find('/');
            if (slash != std::string::npos) {
                local_file = arg.substr(slash + 1);
            }
            if (!retr(arg, local_file)) {
                std::cerr << "Download failed" << std::endl;
            } else {
                std::cout << "Downloaded: " << local_file << std::endl;
            }
        } else if (cmd == "put" || cmd == "STOR") {
            if (arg.empty()) {
                std::cout << "Usage: put <local_file> [remote_file]"
                          << std::endl;
                continue;
            }
            std::string remote_file = arg;
            size_t slash = arg.find('/');
            if (slash != std::string::npos) {
                remote_file = arg.substr(slash + 1);
            }
            if (!stor(arg, remote_file)) {
                std::cerr << "Upload failed" << std::endl;
            } else {
                std::cout << "Uploaded: " << remote_file << std::endl;
            }
        } else if (cmd == "cd" || cmd == "CWD") {
            if (arg.empty()) {
                std::cout << "Usage: cd <directory>" << std::endl;
                continue;
            }
            if (!cwd(arg)) {
                std::cerr << "Change directory failed" << std::endl;
            }
        } else if (cmd == "pwd" || cmd == "PWD") {
            pwd();
        } else if (cmd == "quit" || cmd == "exit") {
            quit();
            break;
        } else {
            std::cout
                << "Unknown command. Available: ls, get, put, cd, pwd, quit"
                << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <server_ip>" << std::endl;
        return 1;
    }

    FTPClient client(argv[1], 2100);
    client.start();

    return 0;
}