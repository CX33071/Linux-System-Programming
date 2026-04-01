#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <thread>
class Socket{
    private:
     int fd_=-1;
    public:
    Socket(int domain,int type,int protocol){
        fd_ = socket(domain, type, protocol);
    }
    ~Socket(){
        if(fd_!=-1){
            close(fd_);
            fd_ = -1;
        }
    }
    int fd() { 
        return fd_;
     }
    bool isvalid() { 
        return fd_ != -1;
     }
    bool enableport() {
    int opt= 1;
    if(setsockopt(fd_,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))==-1){
        std::cout << strerror(errno) << std::endl;
        return false;
    }
    return true;
    }
};
class IPV4{
    private:
     sockaddr_in addr{};
    public:
     IPV4(uint16_t port) { 
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
     }
     sockaddr* change() { 
        return (sockaddr*)&addr;
     }
     socklen_t len() { 
        return sizeof(addr);
     }
     uint16_t getport() {
         return ntohs(addr.sin_port);
        }
};
class Ftpserver {
    private:
     uint16_t port_;
     Socket listensock_;
     int datafd;
     std::string readline(int fd) {
         std::string line;
         char c;
         while (1) {
             ssize_t n = read(fd, &c, 1);
             if (n <= 0) {
                 break;
             }
             line += c;
             if (c == '\n') {
                 break;
             }
         }
         return line;
     }
     std::string getIP() { 
        struct ifaddrs* ifap = nullptr;
        getifaddrs(&ifap);
        std::string s = "127,0,0,1";
        for (auto ifa = ifap;ifa!=nullptr;ifa=ifa->ifa_next){
            if(!ifa->ifa_addr){
                continue;
            }
            if(ifa->ifa_addr->sa_family!=AF_INET){
                continue;
            }
            auto addr = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
            char buf[100]{};
            inet_ntop(AF_INET, &addr->sin_addr, buf, sizeof(buf));
            if(std::string(buf).substr(0,3)=="127"){
                continue;
            }
            for(char&c:buf){
                if (c == '.') {
                    c = ',';
                }
            }
            s = buf;
            break;
        }
        freeifaddrs(ifap);
        return s;
     }
     void sendmessage(int fd, std::string num, std::string s) {
         std::string message;
         message += num;
         message+=' ';
         message += s;
         message += "\r\n";
         write(fd, message.data(), message.size());
     }
     int createPASVsocket(int& datafd) { 
        datafd = socket(AF_INET, SOCK_STREAM, 0);
        IPV4 addr(0);
        bind(datafd, addr.change(), addr.len());
        listen(datafd, 1);
        socklen_t len = addr.len();
        getsockname(datafd, addr.change(), &len);
        int port = addr.getport();
        return port;
     }
     void LIST(int cfd,int datafd){
         int dfd = accept(datafd, nullptr, nullptr);
         close(datafd);
         sendmessage(cfd, "150", "List");
         DIR* dir = opendir(".");
         struct dirent* ent;
         while(ent=readdir(dir)){
             std::string s = ent->d_name;
             s += "\r\n";
             write(dfd, s.data(), s.size());
         }
         closedir(dir);
         close(dfd);
         sendmessage(cfd, "226", "List done");
     }
     void RETR(int cfd,int datafd,std::string s){
         int dfd = accept(datafd, nullptr, nullptr);
         close(datafd);
         int fd = open(s.data(), O_RDONLY);
         if(fd==-1){
             sendmessage(cfd, "550", "File not found");
             close(dfd);
             return;
         }
         sendmessage(cfd, "150", "Send file");
         char buf[1024];
         ssize_t n;
         while((n=read(fd,buf,sizeof(buf)))>0){
             write(dfd, buf, n);
         }
         close(fd);
         close(dfd);
         sendmessage(cfd, "226", "Download done");
     }
     void STOR(int cfd,int datafd,std::string s){
         int dfd = accept(datafd, nullptr, nullptr);
         close(datafd);
         sendmessage(cfd, "150", "Receive file");
         int fd = open(s.data(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
         char buf[1024];
         ssize_t n;
         while((n=read(dfd,buf,sizeof(buf)))>0){
             write(fd, buf, n);
         }
         close(fd);
         close(dfd);
         sendmessage(cfd, "226", "Upload done");
     }
     void handleclient(int cfd) {
         int datafd=-1;
         sendmessage(cfd, "220", "FTP server ready");
         while (1) {
             std::string s = readline(cfd);
             if(s.empty()){
                 break;
             }
             std::string s1, s2;
             size_t space = s.find(' ');
             if(!s.empty()&&s.back()=='\n'){
                 s.pop_back();
             }
             if(!s.empty()&&s.back()=='\r'){
                 s.pop_back();
             }
             if(space==std::string::npos){
                 s1 = s;
                 s2 = "";
             }
             else{
             s1 = s.substr(0, space);
             s2 = s.substr(space + 1);
             }
             if (!strcasecmp(s1.c_str(), "pasv")) {
                 int port = createPASVsocket(datafd);
                 int p1 = port / 256;
                 int p2 = port % 256;
                 char buf[1024];
                 std::string ip = getIP();
                 std::string s = "entering passive mode (";
                 s += ip;
                 s += ',';
                 s += std::to_string(p1);
                 s += ',';
                 s += std::to_string(p2);
                 s += ')';
                 sendmessage(cfd, "227", s);
             } else if (!strcasecmp(s1.c_str(), "list")) {
                 LIST(cfd,datafd);
             } else if (!strcasecmp(s1.c_str(), "retr")) {
                 RETR(cfd,datafd, s2);
             } else if (!strcasecmp(s1.c_str(), "stor")) {
                 STOR(cfd,datafd, s2);
             }
         }
        close(cfd);
     }
    public:
    Ftpserver(uint16_t port):port_(port),listensock_(AF_INET,SOCK_STREAM,0){
        listensock_.enableport();
        IPV4 addr(port_);
        bind(listensock_.fd(), addr.change(), addr.len());
        listen(listensock_.fd(), 10);
        while(1){
            int cfd = accept(listensock_.fd(), nullptr, nullptr);
            std::thread t(&Ftpserver::handleclient, this, cfd);
            t.detach();
            std::string ip = getIP();
            write(datafd, ip.data(), ip.size());
        }
    }
};
int main(){
    Ftpserver server(2100);
    return 0;
}