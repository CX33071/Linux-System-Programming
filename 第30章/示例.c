//静态初始化互斥量
#include <stdio.h>
#include <pthread.h>
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int count = 0;
void*pthread_func(void*arg){
    pthread_mutex_lock(&mutex);
    count += 10;
    pthread_mutex_unlock(&mutex);
    return NULL;
}
int main(){
    pthread_t t1, t2;
    pthread_create(&t1, NULL, pthread_func, NULL);
    pthread_create(&t2, NULL, pthread_func, NULL);
    pthread_join(t1,NULL);
    pthread_join(t2, NULL);
    return 0;
}
//pthread_mutex_trylock
#include <stdio.h>
#include <pthread.h>
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void* pthread_func(void* arg) {
    while(pthread_mutex_trylock(&mutex)!=0){
        printf("线程%d尝试加锁失败\n", pthread_self());
    }
    printf("加锁成功\n");
    return NULL;
}
int main(){
    pthread_t t1, t2;
    pthread_create(&t1, NULL, pthread_func, NULL);
    pthread_create(&t2, NULL, pthread_func, NULL);
    pthread_join(t1, NULL);
    pthread_join(&t2, NULL);
    return 0;
}
//避免死锁,所有线程按照相同顺序，先锁A后锁B
#include <stdio.h>
#include <pthread.h>
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
void* pthread_func1(void* arg) {
    pthread_mutex_lock(&mutex1);
    sleep(1);
    pthread_mutex_lock(&mutex2);
    sleep(1);
    pthread_unlock(&mutex2);
    pthread_unlock(&mutex1);
    return NULL;
}
void*pthread_func2(void*arg){
    pthread_mutex_lock(&mutex1);
    sleep(1);
    pthread_mutex_lock(&mutex2);
    sleep(1);
    pthread_unlock(&mutex2);
    pthread_unlock(&mutex1);
    return NULL;
}
int main(){
    pthread_t t1, t2;
    pthread_create(&t1, NULL, pthread_func1, NULL);
    pthread_create(&t2, NULL, pthread_func2, NULL);
    pthread_join(t1,NULL);
    pthread_join(t2,NULL);
    return 0;
}
//动态初始化互斥锁
#include <stdio.h>
#include <pthread.h>
int main(){
    pthread_mutex_t *mutex= (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(mutex,NULL);
    //
    pthread_mutex_destroy(mutex);
    free(mutex);
    return 0;
}
//递归锁
#include <stdio.h>
#include <pthread.h>
pthread_mutex_t muteX;
void pthread_func(int num) {
    if(num<=0){
        return;
    }
    pthread_mutex_lock(&mutex);
    pthread_func(num - 1);
    pthread_mutex_unlock(&mutex);
}
int main(){
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    // pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex, &attr);
    pthread_func(4);
    pthread_mutex_destroy(&mutex);
    return 0;
}
//条件变量
#include <stdio.h>
#include <pthread.h>
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int flag = 0;
void* child_func(void* arg) {
    pthread_mutex_lock(&mutex);
    while(flag==0){
        pthread_cond_wait(&cond, &mutex);
    }
    printf("子线程收到信号\n");
    pthread_mutex_unlock(&mutex);
    return NULL;
}
int main(){
    pthread_t t1;
    pthread_create(&t1, NULL, child_func, NULL);
    pthred_mutex_lock(&mutex);
    flag = 1;
    pthread_cond_signal(&cond);
    pthred_mutex_unlock(&mutex);
    pthread_join(t1, NULL);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    return 0;
}
//生产者、消费者
#include <pthread.h>
#include <stdio.h>
int count = 0;
pthread_mutex_t not_full;
pthread_mutex_t not_empty;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void* consumer(void* arg) {
    for (int i = 0; i < 10;i++){
        pthread_mutex_lock(&mutex);
        while(count==5){
            pthread_cond_wait(&not_full,)
        }
    }
}
