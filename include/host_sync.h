#define HOST_SYNC

#ifdef HOST_SYNC
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>

#define SHM_NAME "/my_shared_memory"
#define SEM_NAME "/my_semaphore"

struct SyncInfo {
    int    shm_fd;
    void  *shm_ptr;
    sem_t *sem;
};
typedef struct SyncInfo SyncInfo;

// 全局变量，标记是否接收到同步信号
volatile sig_atomic_t signal_received = 0;

// 信号处理函数
void handle_signal(int signo) {
    if (signo == SIGUSR1) {
        signal_received = 1;  // 标记已接收到信号
    }
}

SyncInfo init_sync_env(size_t shm_size) {
    // 获取当前进程号 (PID), 将 PID 写入文件，供 Slave 读取
    pid_t pid = getpid();
    FILE *fp = fopen("/nfs/home/cailuoshan/AISim/Diffsync_Skip/host.pid", "w");
    if (fp) {
        fprintf(fp, "%d\n", pid);
        fclose(fp);
    }
    
    SyncInfo sync_info;
    // 创建共享内存
    sync_info.shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (sync_info.shm_fd == -1) {
        perror("无法创建共享内存");
        return sync_info;
    }

    // 设置共享内存大小
    ftruncate(sync_info.shm_fd, shm_size);

    // 映射共享内存
    sync_info.shm_ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, sync_info.shm_fd, 0);
    if (sync_info.shm_ptr == MAP_FAILED) {
        perror("mmap 失败");
        return sync_info;
    }

    // 创建信号量
    sync_info.sem = sem_open(SEM_NAME, O_CREAT, 0666, 0);
    if (sync_info.sem == SEM_FAILED) {
        perror("信号量创建失败");
        return sync_info;
    }

    // 设置信号处理函数
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);

    return sync_info;
}

void wait_for_sync() {
    printf("主机进程 (%d): 运行到关键位置，等待 SIGUSR1 信号...\n", getpid());
    signal_received = 0;  // 重置标志

    // 挂起进程，等待 SIGUSR1 信号
    while (!signal_received) {
        pause();
    }
}

void transmit_data(SyncInfo sync_info, const void *src_data, size_t datalen) {
    // 传输结果
    memcpy(sync_info.shm_ptr, src_data, datalen);

    // 释放信号量，通知另一个进程
    sem_post(sync_info.sem);
    printf("主机进程: 数据写入共享内存，通知另一个进程读取...\n");
}

void close_sync_env(SyncInfo sync_info, size_t shm_size) {
    // 释放资源
    munmap(sync_info.shm_ptr, shm_size);
    close(sync_info.shm_fd);
    sem_close(sync_info.sem);
    sem_unlink(SEM_NAME);
    shm_unlink(SHM_NAME);
}

void HostSync(const void *src_data, size_t datalen) {
    SyncInfo sync_info = init_sync_env(datalen);
    wait_for_sync();
    transmit_data(sync_info, src_data, datalen);
    close_sync_env(sync_info, datalen);
}

#endif