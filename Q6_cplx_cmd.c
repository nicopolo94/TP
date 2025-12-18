#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define MAX_ARGS 64   // max number of arg

const char WELCOME_MSG[] = "Bienvenue dans le Shell ENSEA.\nPour quitter taper 'exit'\n";
    
int main(void) {
    char buffer[BUFFER_SIZE];
    char prompt[100] = "enseash % ";
    ssize_t bytes_read;
    int status;
    struct timespec start, end;

    char *argv[MAX_ARGS]; // creat a table for arg

    write(STDOUT_FILENO, WELCOME_MSG, strlen(WELCOME_MSG));
    
    while (1) {
        write(STDOUT_FILENO, prompt, strlen(prompt));

        bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE);
        
        buffer[bytes_read - 1] = '\0';

        if (bytes_read == 0) {
            write(STDOUT_FILENO, "\nBye bye...\n", 12);
            break;
        }

        if (bytes_read == 5 && strncmp(buffer, "exit", 4) == 0) {
            write(STDOUT_FILENO, "Bye bye...\n", 11);
            break;
        }

        // parsing the command
        // adding the first word, separator are " " and "\t" 
        argv[0] = strtok(buffer, " \t");

        // if "enter" argv[0] is null
        if (argv[0] == NULL) {
            continue; 
        }

        // for next arg
        int i = 0;
        while (argv[i] != NULL && i < MAX_ARGS - 1) {
            i++;
            argv[i] = strtok(NULL, " \t");
        }

        clock_gettime(CLOCK_REALTIME, &start);

        pid_t pid = fork();

        if (pid == -1) {
            write(STDERR_FILENO, "Error fork\n", 12);
        } 
        else if (pid == 0) {
            execvp(argv[0], argv); // using execvp instead of execlp to run a table of arg
            _exit(EXIT_FAILURE); 
        } 
        else {
            wait(&status);

            clock_gettime(CLOCK_REALTIME, &end);

            long seconds = end.tv_sec - start.tv_sec;
            long nanoseconds = end.tv_nsec - start.tv_nsec;
            long elapsed_ms = (seconds * 1000) + (nanoseconds / 1000000);

            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
                sprintf(prompt, "enseash [exit:%d|%ld ms] %% ", exit_code, elapsed_ms); 
            } 
            else if (WIFSIGNALED(status)) {
                int signal_code = WTERMSIG(status);
                sprintf(prompt, "enseash [sign:%d|%ld ms] %% ", signal_code, elapsed_ms);
            }
        }
    }

    return 0;
}