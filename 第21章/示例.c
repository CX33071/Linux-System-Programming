//在信号处理函数中执行非本地跳转
#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
// static sigjmp_buf env;
// void handle_sigint(int sig){
//     siglongjmp(env, 1);
// }
// int main(){
//     if(signal(SIGINT,handle_sigint)==SIG_ERR){
//         perror("signal");
//         return 1;
//     }
//     if(sigsetjmp(env,1)==0){
//         printf("ctrl+c to break\n");
//         while(1){
//             sleep(1);
//         }
//     }else{
//         printf("caught SIGINT");
//     }
//     return 0;
// }
//abort()
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
void handle_sigabrt(int sig) {
    printf("Caught SIGABRT, but abort() will still terminate me.\n");
}
int main() {
    if (signal(SIGABRT, handle_sigabrt) == SIG_ERR) {
        perror("signal");
        return 1;
    }
    printf("Calling abort()...\n");
    abort();
    // 以下代码永远不会执行
    printf("This line is unreachable.\n");
    return 0;
}

