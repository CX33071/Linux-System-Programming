//设置栈大小
#include <stdio.h>
#include <pthread.h>
#define STACK_SIZE (1024*1024)
void*pthread_func(void*arg){
    
}
int main(){
    pthread_attr_t attr;
    pthread_t t1;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, STACK_SIZE);
    pthread_create(&t1, NULL, pthread_func, NULL);
    pthread_join(t1, NULL);
    pthread_attr_destroy(&attr);
    return 0;
}
//线程处理信号
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
// void*signal_pthread_func(void*arg){
//     sigaddset(&signal_mask, SIGINT);
//     sigaddset(&signal_mask, SIGTERM);
//     pthread_mask(SIG_UNBLOCK, &signal_mask, NULL);
//     int sig;
//     while(1){
//         sigwait(&signal_mask, &sig);
//         if(sig==SIGINT){
//             break;
//         }else if(sig==SIGTERM){
//             break;
//         }
//     }
//     reeturn NULL;
// }
// int main(){
//     sigset_t signal_mask;
//     pthread_t signal_pthread;
//     sigfillset(&signal_mask);
//     pthread_sigmask(SIG_SETMASK, &signal_mask, NULL);
//     pthread_create(&signal_pthread, NULL, signal_pthread_func, NULL);
//     while(1){
//         sleep(1);
//     }
//     pthread_join(signal_pthread, NULL);
//     return 0;
// }
