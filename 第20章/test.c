#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// 使用 volatile sig_atomic_t 保证信号安全
volatile sig_atomic_t got_signal = 0;

void handle_sigusr1(int sig) {
    got_signal = 1;
    // 安全地写入标准错误
    const char msg[] = "收到 SIGUSR1 信号\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
}

int main() {
    if (signal(SIGUSR1, handle_sigusr1) == SIG_ERR) {
        perror("signal");
        return 1;
    }

    printf("等待信号... PID: %d\n", getpid());
    while (!got_signal) {
        pause();  // 暂停进程，直到收到信号
    }
    printf("主程序继续执行\n");
    return 0;
}
