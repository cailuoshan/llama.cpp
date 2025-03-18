#define SLAVE_SYNC

#ifdef SLAVE_SYNC
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define VIRTIO_PATH "/dev/vport1p1"

struct SyncInfo {
    int  virtio_fd;
};
typedef struct SyncInfo SyncInfo;

SyncInfo init_sync_env() {
    SyncInfo sync_info;
    // 打开 virtio 设备文件，从而连接 qemu 创建的 socket 端口
    sync_info.virtio_fd = open(VIRTIO_PATH, O_RDWR);
    if (sync_info.virtio_fd < 0) {
        perror("Can't open VirtIO device");
    }
    return sync_info;
}

void send_req_to_host(SyncInfo sync_info) {
    char msg[] = "DATA_REQ";
    write(sync_info.virtio_fd, msg, strlen(msg));
    printf("\nQEMU: Send req to wake up host.\n");
}

void receive_data(SyncInfo sync_info, void *dest_data, size_t data_len) {
    // 单次传输大小受限于 Linux PIPE_BUF(4096 Bytes)，通过循环 read 接收所有数据
    size_t total_received = 0;
    while (total_received < data_len) {
        ssize_t received = read(sync_info.virtio_fd, dest_data + total_received, data_len - total_received);
        if (received < 0) {
            perror("QEMU: Fail to read data from VirtIO");
            return;
        } else if (received == 0) {
            printf("QEMU: Disconnected from Host.\n");
            return;
        }
        total_received += received;
    }
    printf("QEMU: Finish receive %zu bytes data.\n", total_received);
}

void close_sync_env(SyncInfo sync_info) {
    close(sync_info.virtio_fd);
}

void SlaveSync(void *dest_data, size_t data_len) {
    SyncInfo sync_info = init_sync_env();
    send_req_to_host(sync_info);
    receive_data(sync_info, dest_data, data_len);
    close_sync_env(sync_info);
}

#endif