#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 64

void display_prompt() {
    printf("PUCITshell:- ");
    fflush(stdout);  
}

void parse_command(char *input, char **args) {
    int i = 0;
    args[i] = strtok(input, " \t\n");  
    while (args[i] != NULL) {
        i++;
        args[i] = strtok(NULL, " \t\n");
    }
}

void execute_command(char **args) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("Execution failed");
        }
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        wait(NULL); 
    }
}

int main() {
    char input[MAX_COMMAND_LENGTH];
    char *args[MAX_ARGS];

    while (1) {
        display_prompt();
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\nExiting shell.\n");
            break;  // Exit on <CTRL+D>
        }
        parse_command(input, args);
        
        if (args[0] == NULL) {
            continue;  // Empty command entered
        }
        
        execute_command(args);
    }
