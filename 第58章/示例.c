// 用C获取本机IP地址
// struct ifaddrs 是 Linux/Unix 系统中获取本机所有网络接口信息的核心结构体
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <ifaddrs.h>
int main(){
    struct ifaddrs *ifap, *ifa;
    char ip[INET_ADDRSTRLEN];
    //获取所有网络接口地址
    if(getifaddrs(&ifap)==-1){
        perror("getifaddrs");
        return 1;
    }
    for (ifa = ifap; ifa != NULL;ifa=ifa->ifa_next){
        if(ifa->ifa_addr==NULL){
            continue;
        }
        if(ifa->ifa_addr->sa_family==AF_INET){
            struct sockaddr_in* sa = (struct sockaddr_in*)ifa->ifa_addr;
            inet_ntop(AF_INET, &sa->sin_addr, ip, sizeof(ip));
            printf("interface:%s,IP address:%s\n", ifa->ifa_name, ip);
        }
    }
    freeifaddrs(ifap);
    return 0;
}