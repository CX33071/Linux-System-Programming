//获取进程的实际用户和组ID
#include <stdio.h>
#include <unistd.h>
int main(){
    printf("%d\n", getuid());
    printf("%d\n", getgid());
    return 0;
}
//将程序所有者改为root并设置SUID位
// sudo chown root:root test.cc
// sudo chmod u+s test.c
//获取进程的辅助组ID
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
int main(){
    gid_t groups[100];
    int ngroups = getgroups(100, groups);
    if (ngroups == -1) {
        perror("getgroups failed");
        return 1;
    }
for (int i = 0; i < ngroups; i++) {
        printf("%d ", groups[i]);
        }
return 0;
}
//临时放弃特权和恢复特权
#include <stdio.h>
#include <sys/types.h>  
#include <unistd.h>
int main() {
    uid_t ruid, euid, suid;
    if (getresuid(&ruid, &euid, &suid) == -1) {
        perror("getresuid failed");
        return 1;
    }
    printf("初始状态：实际 UID=%d, 有效 UID=%d, 保存的 UID=%d\n", ruid, euid,
           suid);
    if (seteuid(ruid) == -1) {
        perror("seteuid (drop privilege) failed");
        return 1;
    }
    getresuid(&ruid, &euid, &suid);
    printf("放弃特权后：实际 UID=%d, 有效 UID=%d, 保存的 UID=%d\n", ruid, euid,
           suid);
    if (seteuid(suid) == -1) {  
        perror("seteuid (restore privilege) failed");
        return 1;
    }
    getresuid(&ruid, &euid, &suid);
    printf("恢复特权后：实际 UID=%d, 有效 UID=%d, 保存的 UID=%d\n", ruid, euid,
           suid);

    return 0;
}
