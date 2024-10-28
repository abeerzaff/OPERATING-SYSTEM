
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

void execute_command(char *cmd) {
    char *args[256];
    char *commands[10];  // To hold commands split by pipe
    int in_fd = -1, out_fd = -1;
    int i = 0, cmd_count = 0;

    // Split commands by pipe
    commands[cmd_count] = strtok(cmd, "|");
    while (commands[cmd_count] != NULL) {
        cmd_count++;
        commands[cmd_count] = strtok(NULL, "|");
    }

    // Save original file descriptors for stdin and stdout
    int stdin_backup = dup(STDIN_FILENO);
    int stdout_backup = dup(STDOUT_FILENO);

    for (i = 0; i < cmd_count; i++) {
        // Parse the command into arguments
        args[0] = strtok(commands[i], " ");
        int arg_count = 0;

        while (args[arg_count] != NULL) {
            arg_count++;
            args[arg_count] = strtok(NULL, " ");
        }

        // Check for redirection symbols
        for (int j = 0; args[j] != NULL; j++) {
            if (strcmp(args[j], "<") == 0) {
                // Open input file in read-only mode
                in_fd = open(args[j + 1], O_RDONLY);
                if (in_fd < 0) {
                    perror("Error opening input file");
                    return;
                }
                // Redirect stdin to the input file
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
                args[j] = NULL;  // Remove "<" and filename from arguments
            } else if (strcmp(args[j], ">") == 0) {
                // Open output file in write-only mode (create or truncate)
                out_fd = open(args[j + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (out_fd < 0) {
                    perror("Error opening output file");
                    return;
                }
                // Redirect stdout to the output file
                dup2(out_fd, STDOUT_FILENO);
                close(out_fd);
                args[j] = NULL;  // Remove ">" and filename from arguments
            }
        }

        // If not the last command, create a pipe
        int pipe_fds[2];
        if (i < cmd_count - 1) {
            if (pipe(pipe_fds) == -1) {
                perror("Pipe creation failed");
                return;
            }
        }

        // Fork and execute the command
        if (fork() == 0) {
            if (i < cmd_count - 1) {
                // Redirect stdout to the write end of the pipe
                dup2(pipe_fds[1], STDOUT_FILENO);
                close(pipe_fds[1]); // Close write end after duplication
            }
            if (i > 0) {
                // Redirect stdin to the read end of the previous pipe
                dup2(pipe_fds[0], STDIN_FILENO);
                close(pipe_fds[0]); // Close read end after duplication
            }
            execvp(args[0], args);
            perror("Execution failed");  // If exec fails, print error
            exit(EXIT_FAILURE);
        }

        // Close pipe ends in parent process
        if (i < cmd_count - 1) {
            close(pipe_fds[1]); // Close write end in parent
            // Move read end to stdin for next command
            dup2(pipe_fds[0], STDIN_FILENO);
            close(pipe_fds[0]); // Close read end in parent
        }
        wait(NULL); // Wait for the child process to finish
    }

    // Restore original stdin and stdout
    dup2(stdin_backup, STDIN_FILENO);
    dup2(stdout_backup, STDOUT_FILENO);

    // Close backup file descriptors
    close(stdin_backup);
    close(stdout_backup);
}

int main() {
    char cmd[256];
    
    while (1) {
        printf("PUCITShell@/home/ammar/:- ");
        fflush(stdout);
        
        if (fgets(cmd, sizeof(cmd), stdin) == NULL) break;
        cmd[strcspn(cmd, "\n")] = 0;  // Remove newline from input
        
        if (strcmp(cmd, "exit") == 0) break;
        
        execute_command(cmd);
    }

    return 0;
}
