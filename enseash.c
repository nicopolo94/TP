#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>

#define BUFFER_SIZE 1024

const char WELCOME_MSG[] = "Bienvenue dans le Shell ENSEA.\n";

int main(void) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    int status;

    // print the welcome message
    write(STDOUT_FILENO, WELCOME_MSG, strlen(WELCOME_MSG)); // STDOUT_FILENO is the shell descriptor
    
    return 0;
}