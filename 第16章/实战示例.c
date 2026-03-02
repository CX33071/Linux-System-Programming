// 查看文件的拓展属性
// getfattr test.txt
//设置和获取拓展属性
#include <stdio.h>
#include < sys / xattr.h > 
#include < string.h > 
int main() {
    const char* file = "xattr_demo.txt";
    const char* key = "user.comment";
    const char* value = "
        这是一个测试扩展属性 ";
        if(setxattr(file, key, value, strlen(value), 0) == -1) {
        perror("setxattr failed");
        return 1;
    }
    printf("设置扩展属性成功：%s=%s\n", key, value);
    char buf[100];
    ssize_t len = getxattr(file, key, buf, sizeof(buf));
    if (len == -1) {
            perror("getxattr failed");
            return 1;
        }
    buf[len] = '\0';
    printf("获取扩展属性值：%s=%s\n", key, buf);
    char list[100];
    len = listxattr(file, list, sizeof(list));
    if (len == -1) {
        perror("listxattr failed");
        return 1;
        }
    if (removexattr(file, key) == -1) {
        perror("removexattr failed");
        return 1;
        }
return 0;
}
