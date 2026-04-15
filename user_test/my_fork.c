#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/prctl.h>


int main() {
    // there seems to be a bug with this but oh well
    if (prctl(PR_SET_NAME, "fork_test", 0, 0, 0) < 0) {
        perror("prctl(PR_SET_NAME)");
        exit(1);
    }

    pid_t pid = fork();

    if (pid < 0) {
        // Error occurred
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        // Child process
        printf("This is the child process.\n");
    } else {
        // Parent process
        printf("This is the parent process. Child PID: %d\n", pid);
    }

    return 0;
}
