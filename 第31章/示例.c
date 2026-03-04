//确保初始化函数仅调用1次
#include <stdio.h>
#include <pthread.h>
pthread_once_t once_control = PTHREAD_ONCE_INIT;
void init() {
    printf("初始化");
}
void*pthread_func(void*arg){
    pthread_once(&once_control, init);
    return NULL;
}
int main(){
    pthread_t t1, t2;
    pthread_create(&t1, NULL, pthread_func, NULL);
    pthread_create(&t2, NULL, pthread_func, NULL);
    pthread_join(t1,NULL);
    pthread_join(t2,NULL);
    return 0;
}
//