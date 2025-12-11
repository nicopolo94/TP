#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024

const char WELCOME_MSG[] = "Bienvenue dans le Shell ENSEA.\nPour quitter taper 'exit'\n";
const char PROMPT[] = "enseash % ";

int main(void) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    int status;

    write(STDOUT_FILENO, WELCOME_MSG, strlen(WELCOME_MSG));
    
    while (1) {
        write(STDOUT_FILENO, PROMPT, strlen(PROMPT));

        bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE);

        // for <ctrl>+d
        if (bytes_read == 0) { // bytes_read is 0 when Ctrl+D
            write(STDOUT_FILENO, "\nBye bye...\n", 12);
            break; //exit while loop
        }

        // for command exit
        if (strncmp(buffer, "exit", 4) == 0) { // string compare between buffer and 'exit'
            write(STDOUT_FILENO, "Bye bye...\n", 11);
            break;
        }

        pid_t pid = fork();

        if (pid == -1) {
            write(STDERR_FILENO, "Error fork\n", 12);
        } 
        else if (pid == 0) {
            execlp(buffer, buffer, (char *)NULL);
            _exit(EXIT_FAILURE); 
        } 
        else {
            wait(&status);
        }
    }

    return 0;
}