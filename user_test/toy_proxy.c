/**
 * @file toy_proxy.c
 * @author Faisal Abdelmonem (fts@alumni.cmu.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-31
 * 
 * @copyright Copyright (c) 2025
 * 
 * the way we test this is by having four terminals
 * 1: the kernel module of course (insert the module, then load wasm_probes/connect_counter.wasm)
 * 2: nc -lk 9000 (server side) 
 * 3: ./user_test/toyproxy enable/disable (to run the toy proxy)
 * 4: nc localhost 8080
 * 
 * then you can write your messages on the localhost and verify they are
 * being listned to and echoed on the server (terminal 1).
 * kill the localhost and the backend server and rerun them again N times
 * after that you can call report in terminal which will count the number of times connect was called.
 * when the cache is enabled you should see 1, when it is disabled you should see N.
 * 
 * The test case is also available in test/connect_counter_test.sh
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/prctl.h>

#define LISTEN_PORT 8080
#define BACKEND_PORT 9000
#define BACKEND_IP "127.0.0.1"
#define MAX_CONN 10
#define BUFFER_SIZE 4096

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Connect once to backend, return fd or -1
int connect_backend() {
    int backend_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(backend_fd < 0) {
        perror("socket backend");
        return -1;
    }
    struct sockaddr_in backend_addr;
    memset(&backend_addr, 0, sizeof(backend_addr));
    backend_addr.sin_family = AF_INET;
    backend_addr.sin_port = htons(BACKEND_PORT);
    inet_pton(AF_INET, BACKEND_IP, &backend_addr.sin_addr);

    if(connect(backend_fd, (struct sockaddr*)&backend_addr, sizeof(backend_addr)) < 0) {
        perror("connect backend");
        close(backend_fd);
        return -1;
    }
    return backend_fd;
}

void proxy_loop(int listen_fd, int enabled) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd;
    int cached_backend_fd = -1;  // Used only in enabled mode

    if(enabled) {
        cached_backend_fd = connect_backend();
        if(cached_backend_fd < 0) {
            fprintf(stderr, "Failed to create cached backend connection\n");
            exit(1);
        }
        set_nonblocking(cached_backend_fd);
        // printf("Cached backend connection established (fd=%d)\n", cached_backend_fd);
    }

    while(1) {
        client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
        if(client_fd < 0) {
            if(errno == EINTR) continue;
            perror("accept");
            continue;
        }
        // printf("Accepted client connection (fd=%d)\n", client_fd);

        int backend_fd;
        if(enabled) {
            // Proxy enabled: reuse cached backend connection
            backend_fd = cached_backend_fd;
            // printf("Using cached backend connection (fd=%d)\n", backend_fd);
        } else {
            // Proxy disabled: open new backend connection for this client
            backend_fd = connect_backend();
            if(backend_fd < 0) {
                close(client_fd);
                continue;
            }
            set_nonblocking(backend_fd);
            // printf("Opened new backend connection (fd=%d)\n", backend_fd);
        }

        set_nonblocking(client_fd);

        struct pollfd fds[2];
        char buffer[BUFFER_SIZE];
        int n;

        while(1) {
            fds[0].fd = client_fd;
            fds[0].events = POLLIN;
            fds[1].fd = backend_fd;
            fds[1].events = POLLIN;

            int ret = poll(fds, 2, 30000); // 30 sec timeout
            if(ret <= 0) {
                printf("poll timeout or error\n");
                break;
            }

            // Client -> Backend
            if(fds[0].revents & POLLIN) {
                n = read(client_fd, buffer, BUFFER_SIZE);
                if(n <= 0) break;
                write(backend_fd, buffer, n);
            }

            // Backend -> Client
            if(fds[1].revents & POLLIN) {
                n = read(backend_fd, buffer, BUFFER_SIZE);
                if(n <= 0) break;
                write(client_fd, buffer, n);
            }
        }

        close(client_fd);

        if(!enabled) {
            // Close per-client backend connection in disabled mode
            close(backend_fd);
            // printf("Closed per-client backend connection (fd=%d)\n", backend_fd);
        } else {
            // In enabled mode, keep cached backend connection open!
            // But if backend disconnected, reconnect:
            if(fds[1].revents & (POLLHUP | POLLERR)) {
                // printf("Cached backend connection closed, reconnecting...\n");
                close(cached_backend_fd);
                cached_backend_fd = connect_backend();
                if(cached_backend_fd < 0) {
                    fprintf(stderr, "Failed to reconnect cached backend\n");
                    exit(1);
                }
                set_nonblocking(cached_backend_fd);
            }
        }

        // printf("Client connection closed\n");
    }
}

int main(int argc, char *argv[]) {
    int enabled = 1;

    if (prctl(PR_SET_NAME, "toyproxy", 0, 0, 0) < 0) {
        perror("prctl(PR_SET_NAME)");
        exit(1);
    }

    if(argc > 1) {
        if(strcmp(argv[1], "disable") == 0) {
            enabled = 0;
        } else if(strcmp(argv[1], "enable") == 0) {
            enabled = 1;
        } else {
            fprintf(stderr, "Usage: %s [enable|disable]\n", argv[0]);
            exit(1);
        }
    }

    // printf("Starting toy proxy on port %d, connection caching %s\n",
    //        LISTEN_PORT, enabled ? "ENABLED" : "DISABLED");

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_fd < 0) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(LISTEN_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if(listen(listen_fd, MAX_CONN) < 0) {
        perror("listen");
        exit(1);
    }

    proxy_loop(listen_fd, enabled);

    close(listen_fd);
    return 0;
}
