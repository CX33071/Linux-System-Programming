//环境变量相关API
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
int main(){
    extern char** environ;
    for (int i = 0; environ[i] != NULL;i++){
        printf("%s\n", environ[i]);
    }
    char* path = getenv("PATH");
    if(path!=NULL){
        printf("PATH:%s", path);
    }
    int ret = setenv("MY_VAR", "hello", 1);
    if(ret==0){
        printf("设置成功");
    }
    unsetenv("MY_VAR");
    printf("%s", getenv("MY_VAR"));
    return 0;
}