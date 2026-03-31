#include <stdio.h>
#define FENGE '\0'
#define BUF_SIZE 1024
void send(int fd,char *s){
    char buf[BUF_SIZE];
    snprintf(buf, sizeof(buf), "%s%c", s, FENGE);
    write(fd, buf, sizeof(buf));
}
void revc(int fd,char*s,int len){
    int idx = 0;
    char* s1;
    int i = 0;
    while (idx < len - 1) {
        if(s[i]=='\n'){
            s1[idx] = '\0';
            return s;
        } else {
            s1[idx++] = s[i];
        }
        i++;
    }
}