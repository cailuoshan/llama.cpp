#define HOST_SYNC

#ifdef HOST_SYNC
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h> 
#include <sys/un.h>

#define SOCKET_PATH "/tmp/virtio-serial.sock"

struct SyncInfo {
    int  socket_fd;
};
typedef struct SyncInfo SyncInfo;

SyncInfo init_sync_env() {
    SyncInfo sync_info;
    // 创建 Socket 并配置 Socket 地址
    sync_info.socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr = { .sun_family = AF_UNIX, .sun_path = SOCKET_PATH };

    // 连接到 QEMU 提供的 Socket
    if (connect(sync_info.socket_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("connect");
        close(sync_info.socket_fd);
        return sync_info;
    }

    return sync_info;
}

void wait_for_sync(SyncInfo sync_info) {
    printf("Host: Sync_data ready, waiting for QEMU DATA_REQ...\n");
    char msg[16]; 
    ssize_t msglen = read(sync_info.socket_fd, msg, 16);
    if (msglen > 0) {
        if (strcmp(msg, "DATA_REQ") == 0) {
            printf("\nHost: Receive DATA_REQ from QEMU, start transmitting...\n");
        }
    } else if (msglen == 0) {
        printf("Disconnected from QEMU.\n");
    } else {
        perror("read");
    }
}

void transmit_data(SyncInfo sync_info, const void *src_data, size_t data_len) {
    // 单次传输大小受限于 Linux PIPE_BUF(4096 Bytes)，通过循环 write 传输所有数据
    size_t total_sent = 0;
    while (total_sent < data_len) {
        ssize_t sent = write(sync_info.socket_fd, src_data + total_sent, data_len - total_sent);
        if (sent < 0) {
            perror("write");
            return;
        }
        total_sent += sent;
    }
    printf("Host: Finish send %zu bytes data.\n", total_sent);
}

void close_sync_env(SyncInfo sync_info) {
    close(sync_info.socket_fd);
}

void HostSync(const void *src_data, size_t data_len) {
    SyncInfo sync_info = init_sync_env();
    wait_for_sync(sync_info);
    transmit_data(sync_info, src_data, data_len);
    close_sync_env(sync_info);
}

#endif