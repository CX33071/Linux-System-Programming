//按用户名获取用户信息
#include <stdio.h>
#include <pwd.h>
int main(){
    char* username = "root";
    struct passwd* pwd = getpwnam(username);
    if(pwd==NULL){
        perror("getpwnam failed");
        return 1;
    }
    printf("%s", pwd->pw_name);
    printf("%d", pwd->pw_uid);
    return 0;
}
//遍历获取用户信息
#include <stdio.h>
#include <pwd.h>
int main(){
    struct passwd* pwd;
    setpwent();
    while((pwd=getpwent())!=NULL){
        printf("%s", pwd->pw_name);
    }
    endpwent();
    return 0;
}
//验证用户密码，模拟登陆认证
#include <stdio.h>
#include <shadow.h>
#include <crypt.h>
