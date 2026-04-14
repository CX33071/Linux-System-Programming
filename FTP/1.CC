#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <string>
void getip(sockaddr_in addr) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    char ip[100];
    socklen_t len = sizeof(addr);
    getsockname(fd, (sockaddr*)&addr, &len);
    inet_ntop(AF_INET, &addr, ip, sizeof(ip));
    std::string ret = ip;
    std::cout << ret;
}