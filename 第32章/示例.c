//取消线程
#include <stdio.h>
#include <pthread.h>
void*pthread_func(void*arg){
    sleep(3);
}
int main(){
    pthread_t t1;
    pthread_create(&t1, NULL, pthread_func, NULL);
    pthread_cancle(t1);
    void* ret;
    pthread_join(t1, &ret);
    if(ret==PTHREAD_CANCELED){
        PRINTF("子线程被成功取消");
    }
    return 0;
}
//禁用取消状态
#include <stdio.h>
#include <pthread.h>
void*pthread_func(void*arg){
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    printf("子进程执行关键任务，取消已禁用");
    sleep(3);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    sleep(1);
    return NULL;
}
int main(){
    pthread_t t1;
    pthread_create(&t1, NULL, pthread_func, NULL);
    pthread_cancel(t1);
    void* ret;
    pthread_join(t1, &ret);
    if(ret==PTHREAD_CANCELED){
        printf("子进程被成功取消");
    }
    return 0;
}
//手动取消点
#include <stdio.h>
#include <pthread.h>
void*pthread_func(void*arg){
    printf("1");
    pthread_testcancel();
}
int main(){
    pthread_t t1;
    pthread_create(&t1, NULL, pthread_func, NULL);
    pthread_cancel(t1);
    void* ret;
    pthread_join(t1, &ret);
    if(ret==PTHREAD_CANCELED){
        printf("子进程被成功取消");
    }
    return 0;
}
//取消时释放锁
#include <stdio.h>
#include <pthread.h>
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void unlock_handler(void*arg){
    pthread_mutex_unlock((pthread_mutex_t*)arg);
}
void*pthread_func(void*arg){
    pthread_mutex_lock(&mutex);
    pthread_cleanup_push(unlock_handler, &mutex);
    sleep(5);
    pthread_cleanup_pop(0);
}
int main(){
    pthread_t t1;
    pthread_mutex_t mutex;
    pthread_create(&t1, NULL, pthread_func, NULL);
    pthread_cancel(t1);
    void* ret;
    pthread_join(t1, &ret);
    if(ret==PTHREAD_CANCELED){
        printf("子进程被成功取消");
    }
    return 0;
}