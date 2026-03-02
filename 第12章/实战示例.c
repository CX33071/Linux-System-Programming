//读取/proc/cpuinfo获取CPU核心参数
#include <stdio.h>
#include <string.h>
int main(){
    FILE* fp = fopen("/proc/cpuinfo", "r");
    if(fp==NULL){
        perror("fopen failed");
        return 1;
    }
    char line[256];
    int count = 0;
    while(fgets(line,sizeof(line),fp)!=NULL){
        if(strstr(line,"processor\t:")!=NULL){
            count++;
        }
    }
    fclose(fp);
    printf("CPU核心数:%d\n", count);
    return 0;
}
//读取/proc/self/status获取进程内存信息
#include <stdio.h>
#include <string.h>
int main(){
    char line[256];
    FILE* fp = fopen("/proc/self/status", "r");
    if(fp==NULL){
        perror("fopen failed");
        return 1;
    }
    while(fgets(line,sizeof(line),fp)!=NULL){
        if(strstr(line,"VmSize")!=NULL||strstr(line,"VmRss")!=NULL){
            printf("%s", line);
        }
    }
    fclose(fp);
    return 0;
}
//读取/proc/loadavg查看系统负载
#include <stdio.h>
int main(){
    FILE* fp = fopen("/proc/loadavg", "r");
    if(fp==NULL){
        perror("fopen failed");
        return 1;
    }
    double load1, load5, load15;
    fscanf(fp, "%lf %lf %lf", &load1, &load5, &load15);
    fclose(fp);
    return 0;
}
//修改内核参数/proc/sys/net/ipv4/ip_forward
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
int main(){
    int fd = open("/proc/sys/net/ipv4/ip_forward", O_WRONLY);
    if(fd==-1){
        perror("open failed");
        return 1;
    }
    write(fd, "1", 1);
    clode(fd);
    printf("已开启IP转发\n");
    return 0;
}
//获取系统标识信息
#include <stdio.h>
#include <sys/utsname.h>
int main(){
    struct utsname sys_info;
    if(uname(&sys_info)==-1){
        perror("uname failed");
        return 1;
    }
    printf("操作系统:%s\n", sys_info.sysname);
    printf("主机名:%s\n", sys_info.nodename);
    printf("内核版本：%s\n", sys_info.release);
    return 0;
}