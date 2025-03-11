#define SLAVE_SYNC

#ifdef SLAVE_SYNC
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>

#define SHM_NAME "/my_shared_memory"
#define SEM_NAME "/my_semaphore"

struct SyncInfo {
    int    shm_fd;
    void  *shm_ptr;
    sem_t *sem;
    pid_t  host_pid;
};
typedef struct SyncInfo SyncInfo;

SyncInfo init_sync_env(size_t shm_size) {
    SyncInfo sync_info;

    // 读取 Host 的 PID
    FILE *fp = fopen("/nfs/home/cailuoshan/AISim/Diffsync_Skip/host.pid", "r");
    if (fscanf(fp, "%d", &(sync_info.host_pid)) != 1) {
        perror("Failed to read PID");
        fclose(fp);
        return sync_info;
    }
    fclose(fp);
    
    // 打开共享内存
    sync_info.shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (sync_info.shm_fd == -1) {
        perror("无法打开共享内存");
        return sync_info;
    }

    // 映射共享内存
    sync_info.shm_ptr = mmap(0, shm_size, PROT_READ, MAP_SHARED, sync_info.shm_fd, 0);
    if (sync_info.shm_ptr == MAP_FAILED) {
        perror("mmap 失败");
        return sync_info;
    }

    // 打开信号量
    sync_info.sem = sem_open(SEM_NAME, 0);
    if (sync_info.sem == SEM_FAILED) {
        perror("信号量打开失败");
        return sync_info;
    }

    return sync_info;
}

void close_sync_env(SyncInfo sync_info, size_t shm_size) {
    // 释放资源
    munmap(sync_info.shm_ptr, shm_size);
    close(sync_info.shm_fd);
    sem_close(sync_info.sem);
    sem_unlink(SEM_NAME);
    shm_unlink(SHM_NAME);
}

void SlaveSync(void *dest_data, size_t datalen) {
    SyncInfo sync_info = init_sync_env(datalen);
    
    // 发送 SIGUSR1 信号唤醒主进程
    printf("独立进程: 发送 SIGUSR1 信号给 %d\n", sync_info.host_pid);
    kill(sync_info.host_pid, SIGUSR1);

    // 等待信号量（确保主进程已经写入数据）
    sem_wait(sync_info.sem);

    // 将数据拷贝至目标地址
    memcpy(dest_data, sync_info.shm_ptr, datalen);

    close_sync_env(sync_info, datalen);
    sleep(1);
}

#endif