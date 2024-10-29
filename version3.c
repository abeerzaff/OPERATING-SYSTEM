#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>

void handle_sigchld(int sig) {
    // Reap all terminated child processes to avoid zombies
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    char command[256];
    char *args[10];
    pid_t pid;

    // Set up signal handler for SIGCHLD to avoid zombies
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    while (1) {
        printf("PUCITshell@/home/user:- ");
        fgets(command, 256, stdin);

        // Remove newline character at the end of the input
        command[strcspn(command, "\n")] = '\0';

        // Check for "&" at the end to decide if the process should run in the background
        int background = 0;
        if (command[strlen(command) - 1] == '&') {
            background = 1;
            command[strlen(command) - 1] = '\0';  // Remove the "&" from the command
        }

        // Parse the command into arguments
        int i = 0;
        args[i] = strtok(command, " ");
        while (args[i] != NULL) {
            args[++i] = strtok(NULL, " ");
        }

        // Fork a child process to execute the command
        pid = fork();
        if (pid == 0) {
            // In the child process: execute the command
            if (execvp(args[0], args) == -1) {
                perror("Error executing command");
            }
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            // In the parent process
            if (background) {
                // Print the background process ID
                printf("[%d] %d\n", i, pid);
            } else {
                // Wait for the child process to complete if not in background
                waitpid(pid, NULL, 0);
            }
        } else {
            // If fork failed
            perror("Failed to fork");
        }
    }

    return 0;
}
