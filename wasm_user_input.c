/**
 * @file wasm_user_input.c
 * @author Faisal Abdelmonem (fts@alumni.cmu.edu)
 * @brief 
 * @version 0.1
 * @date 2025-02-06
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#include "chardev_config.h"
#include "wasm_probes/execution_counter.h"

void print_usage(const char *prog_name) {
    printf("Usage:\n");
    printf("  %s load <wasm_module.wasm>\n", prog_name);
    printf("  %s activate <id>\n", prog_name);
    printf("  %s deactivate <id>\n", prog_name);
    printf("  %s unload <id>\n", prog_name);
    printf("  %s report <id>\n", prog_name);
    printf("  %s list\n", prog_name);
}

unsigned char* read_wasm_file(const char* filename, size_t* size) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("fopen");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    rewind(file);

    unsigned char* buffer = malloc(*size);
    if (!buffer) {
        perror("malloc");
        fclose(file);
        return NULL;
    }

    if (fread(buffer, 1, *size, file) != *size) {
        perror("fread");
        free(buffer);
        fclose(file);
        return NULL;
    }

    fclose(file);
    return buffer;
}

// we get the wasm binary by compiling our file with:
// clang --target=wasm32 -nostdlib -Wl,--no-entry -Wl,--export-all, -Wl,--global-base=1024 -o mkdir_counter.wasm mkdir_counter.c 
int main(int argc, char *argv[]) {
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    
    int id;
    unsigned char* wasm_binary;

    if (strcmp(argv[1], "help") == 0) {
        print_usage(argv[1]);
    } else if (strcmp(argv[1], "load") == 0) {
        struct wasm_load_request req;
        strncpy(req.path, argv[2], MAX_WASM_PATH - 1);
        req.path[MAX_WASM_PATH - 1] = '\0';

        req.binary = read_wasm_file(argv[2], &req.size);
        if (!req.binary) {
            close(fd);
            return 1;
        }

        // memcpy(req.binary, wasm_binary, req.size);
        // req.binary = wasm_binary;
        
        if ((id = ioctl(fd, IOCTL_LOAD, &req)) < 0) {
            perror("ioctl load");
        } else {
            printf("Loaded WASM with ID: %d\n", id);
        }
        free(req.binary);
    } else if (strcmp(argv[1], "activate") == 0) {
        id = atoi(argv[2]);
        if (ioctl(fd, IOCTL_ACTIVATE, id) < 0) {
            perror("ioctl activate");
        } else {
            printf("Activated WASM ID: %d\n", id);
        }
    } else if (strcmp(argv[1], "deactivate") == 0) {
        id = atoi(argv[2]);
        if (ioctl(fd, IOCTL_DEACTIVATE, id) < 0) {
            perror("ioctl deactivate");
        } else {
            printf("Deactivated WASM ID: %d\n", id);
        }
    } else if (strcmp(argv[1], "unload") == 0) {
        id = atoi(argv[2]);
        if (ioctl(fd, IOCTL_UNLOAD, id) < 0) {
            perror("ioctl unload");
        } else {
            printf("Unloaded WASM ID: %d\n", id);
        }
    } else if (strcmp(argv[1], "report") == 0){
        id = atoi(argv[2]);
        if (ioctl(fd, IOCTL_REPORT, id) < 0) {
            perror("ioctl report");
        } else {
            char buffer[128];
            int bytes_read = read(fd, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                printf("Report: %s", buffer);
            } else {
                perror("read");
            }
        }
    } else if (strcmp(argv[1], "list") == 0) {
        char buf[BUFFER_LEN];
        if (ioctl(fd, IOCTL_LIST, buf) == 0) {
            printf("Loaded modules:\n%s", buf);
        } else {
            perror("ioctl list");
        }
    } else if (strcmp(argv[1], "info") == 0) {
        struct wasm_request req;
        req.id = atoi(argv[2]);
        if (ioctl(fd, IOCTL_INFO, &req) == 0) {
            printf("Module Info:\n%s", req.buffer);
        } else {
            perror("ioctl list");
        }
    } else {
        print_usage(argv[0]);
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}
