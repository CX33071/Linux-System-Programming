//正确使用realloc
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(){
    char* buf = malloc(10);
    if(buf==NULL){
        perror("mallloc");
        return 1;
    }
    strcpy(buf, "hello");
    char* new_buf = realloc(buf, 20);
    if(new_buf==NULL){
        perror("realloc");
        free(buf);
        return 1;
    }
    buf = new_buf;
    strcat(buf, "world");
    free(buf);
    buf = NULL;
}