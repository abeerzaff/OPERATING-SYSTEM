#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_COMMAND_LENGTH 256
#define MAX_ARGS 10

void handle_sigchld(int sig) {
    // Reap all terminated child processes
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void display_prompt() {
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("PUCITshell@%s:- ", cwd);
    fflush(stdout);
}

void parse_command(char *command, char **args, int *background) {
    *background = 0;
    if (command[strlen(command) - 1] == '&') {
        *background = 1;
        command[strlen(command) - 1] = '\0'; // Remove '&'
    }
    int i = 0;
    args[i] = strtok(command, " ");
    while (args[i] != NULL && i < MAX_ARGS - 1) {
        args[++i] = strtok(NULL, " ");
    }
    args[i] = NULL;
}

void execute_single_command(char **args, int background, char *infile, char *outfile) {
    pid_t pid = fork();

    if (pid == 0) { // Child process
        // Handle input redirection
        if (infile) {
            int fd_in = open(infile, O_RDONLY);
            if (fd_in < 0) { perror("Input file error"); exit(1); }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }
        
        // Handle output redirection
        if (outfile) {
            int fd_out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out < 0) { perror("Output file error"); exit(1); }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }
        
        if (execvp(args[0], args) == -1) {
            perror("Error executing command");
            exit(EXIT_FAILURE);
        }
    } else if (pid > 0) { // Parent process
        if (background) {
            printf("[%d] %d\n", getpid(), pid);
        } else {
            waitpid(pid, NULL, 0); // Wait for foreground process to finish
        }
    } else {
        perror("Fork failed");
    }
}

void handle_pipes_and_redirection(char *command) {
    char *args[MAX_ARGS];
    int background = 0;
    char *infile = NULL, *outfile = NULL;
    
    // Check for pipes
    char *cmd1 = strtok(command, "|");
    char *cmd2 = strtok(NULL, "|");

    if (cmd2) { // Handle piping
        int pipefd[2];
        pipe(pipefd);

        if (fork() == 0) { // First child for cmd1
            dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to pipe
            close(pipefd[0]);
            close(pipefd[1]);

            parse_command(cmd1, args, &background);
            execvp(args[0], args);
            perror("Exec failed for cmd1");
            exit(EXIT_FAILURE);
        }

        if (fork() == 0) { // Second child for cmd2
            dup2(pipefd[0], STDIN_FILENO); // Redirect stdin to pipe
            close(pipefd[1]);
            close(pipefd[0]);

            parse_command(cmd2, args, &background);
            execvp(args[0], args);
            perror("Exec failed for cmd2");
            exit(EXIT_FAILURE);
        }

        close(pipefd[0]);
        close(pipefd[1]);
        wait(NULL);
        wait(NULL);
        
    } else { // Handle I/O redirection
        char *input = strstr(command, "<");
        char *output = strstr(command, ">");
        
        if (input) {
            *input = '\0';
            infile = strtok(input + 1, " ");
        }
        if (output) {
            *output = '\0';
            outfile = strtok(output + 1, " ");
        }

        parse_command(command, args, &background);
        execute_single_command(args, background, infile, outfile);
    }
}

int main() {
    char command[MAX_COMMAND_LENGTH];
    char *args[MAX_ARGS];
    int background;

    // Set up signal handler for SIGCHLD to avoid zombies
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    while (1) {
        display_prompt();

        // Read command input
        if (fgets(command, sizeof(command), stdin) == NULL) {
            printf("\nExiting shell...\n");
            break; // Exit on <CTRL+D>
        }

        // Remove newline character
        command[strcspn(command, "\n")] = '\0';

        // Handle pipes and redirection
        handle_pipes_and_redirection(command);
    }

    return 0;
}