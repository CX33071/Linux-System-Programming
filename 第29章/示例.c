//创建线程
#include <pthread.h>
#include <stdio.h>
void*pthread_exe(void*arg){
    int num = *(int*)arg;
    printf("线程ID:%d", (unsigned long)pthread_self());//执行函数里调用pthread_self返回创建的子线程ID，如果在主函数里调用pthread_self,返回的是主线程的ID
    return NULL;
}
int main(){
    pthread_t t1, t2;
    int num1 = 1;
    int num2 = 2;
    pthread_create(&t1, NULL, pthread_exe, &num1);
    pthread_create(&t2, NULL, pthread_exe, &num2);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return 0;
}
//主动终止线程
#include <pthread.h>
#include <stdio.h>
volatile __sig_atomic_t num = 100;
void* pthread_func(void* arg) {
    pthread_exit((void*)&num);
    printf("线程主动退出\n");
    return NULL;
}
int main(){
    pthread_t t1;
    void* canshu;
    pthread_create(&t1, NULL, pthread_func, NULL);
    pthread_join(t1, &canshu);
    int* ret = (int*)canshu;
    printf("线程退出返回%d", *ret);
    return 0;
}
//线程的返回值
#include <stdio.h>
#include <pthread.h>
void* num2 = "主动调用退出值";
void* pthread_func1(void* arg1) {
    int *num= (int*)arg1;
    int ret = *num;
    return (void*)ret;
}
void*pthread_func2(void*arg2){
    sleep(1);
    pthread_exit(num2);
    return NULL;
}
int main(){
    pthread_t t1,t2;
    int arg = 1;
    int retval1;
    int retval2;
    pthread_create(&t1, NULL, pthread_func1, &arg);
    pthread_create(&t2, NULL, pthread_func2, NULL);
    pthread_join(t1, (void**)&retval1);
    pthread_join(t2, (void**)&retval2);
    printf("线程%d退出返回值%d", retval1, retval2);
}
//创建分离线程
#include <pthread.h>
#include <stdio.h>
void*pthread_func(void*arg){
    sleep(1);
    return NULL;
}
int main(){
    pthread_t t1, t2;
    pthread_attr_t attr;
    pthread_attr_init(&t1);
    pthread_attr_setdetachstate(&t1,PTHREAD_CREATE_DETACHED);
    pthread_create(&t1, &attr, pthread_func, NULL);
    pthread_attr_destroy(&t1);
    pthread_create(&t2, NULL, pthread_func, NULL);
    if(pthread_detach(t2)!=0){
        perror("pthread_detach failed");
        return 1;
    }
    return 0;
}
//创建线程设置栈大小
#include <pthread.h>
#include <stdio.h>
void*pthread_func(void*arg){
    sleep(1);
    return NULL;
}
int main(){
    pthread_t t1;
    pthread_attr_t attr;
    size_t stack_size = 1024 * 1024;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&t1, stack_size);
    pthread_create(&t1, &attr, pthread_func, &stack_size);
    pthread_attr_destroy(&attr);
    pthread_join(t1,NULL);
    return 0;
}