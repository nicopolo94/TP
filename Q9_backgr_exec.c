#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024
#define MAX_ARGS 64
#define MAX_JOBS 10  // <--- Q9: Max processus en arrière-plan

// <--- Q9: Structure pour gérer un job
typedef struct {
    pid_t pid;
    int id;                 // Numéro du job (ex: [1])
    struct timespec start;  // Heure de lancement
    char command[BUFFER_SIZE]; // La commande pour l'affichage
} Job;

Job jobs[MAX_JOBS];
int jobs_count = 0;

// Fonction pour ajouter un job à la liste
void add_job(pid_t pid, char *command, struct timespec start) {
    if (jobs_count < MAX_JOBS) {
        jobs[jobs_count].pid = pid;
        jobs[jobs_count].id = jobs_count + 1;
        jobs[jobs_count].start = start;
        strncpy(jobs[jobs_count].command, command, BUFFER_SIZE);
        jobs_count++;
        // Affiche [1] 1234
        printf("[%d] %d\n", jobs_count, pid); 
    }
}

// Fonction pour supprimer un job (quand il est fini)
void remove_job(pid_t pid) {
    int found = 0;
    for (int i = 0; i < jobs_count; i++) {
        if (jobs[i].pid == pid) {
            found = 1;
        }
        if (found && i < jobs_count - 1) {
            jobs[i] = jobs[i + 1]; // On décale tout
        }
    }
    if (found) jobs_count--;
}

// (Fonction helpers des questions précédentes)
void manage_redirections(char **argv) {
    for (int j = 0; argv[j] != NULL; j++) {
        if (strncmp(argv[j], ">", 1) == 0) {
            char *filename = argv[j+1];
            if (!filename) exit(EXIT_FAILURE);
            int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(fd, STDOUT_FILENO); close(fd);
            argv[j] = NULL; j++;
        } else if (strncmp(argv[j], "<", 1) == 0) {
            char *filename = argv[j+1];
            if (!filename) exit(EXIT_FAILURE);
            int fd = open(filename, O_RDONLY);
            dup2(fd, STDIN_FILENO); close(fd);
            argv[j] = NULL; j++;
        }
    }
}

pid_t exec_command(char **argv, int input_fd, int output_fd) {
    pid_t pid = fork();
    if (pid == 0) {
        if (input_fd != STDIN_FILENO) { dup2(input_fd, STDIN_FILENO); close(input_fd); }
        if (output_fd != STDOUT_FILENO) { dup2(output_fd, STDOUT_FILENO); close(output_fd); }
        manage_redirections(argv);
        execvp(argv[0], argv);
        perror("execvp"); exit(EXIT_FAILURE);
    }
    return pid;
}

int main(void) {
    char buffer[BUFFER_SIZE];
    char prompt[100] = "enseash % ";
    char raw_command[BUFFER_SIZE]; // Copie pour stocker le nom de la commande
    char *argv[MAX_ARGS]; 
    struct timespec start, end;
    int status;

    write(STDOUT_FILENO, "Bienvenue dans le Shell ENSEA.\nPour quitter, tapez 'exit'.\n", 60);

    while (1) {
        // <--- Q9: CHECK DES JOBS FINIS AVANT LE PROMPT
        // On vérifie si des fils en arrière-plan sont morts
        pid_t z_pid;
        while ((z_pid = waitpid(-1, &status, WNOHANG)) > 0) {
            // Un fils est mort, on le cherche dans notre liste
            for (int i = 0; i < jobs_count; i++) {
                if (jobs[i].pid == z_pid) {
                    clock_gettime(CLOCK_REALTIME, &end);
                    long ms = (end.tv_sec - jobs[i].start.tv_sec) * 1000 + 
                              (end.tv_nsec - jobs[i].start.tv_nsec) / 1000000;
                    
                    // Affichage type: [1]+ Ended: sleep 10 &
                    printf("[%d]+ Ended: %s\n", jobs[i].id, jobs[i].command);
                    
                    // Mise à jour du prompt avec le temps de ce background job
                    if (WIFEXITED(status))
                        snprintf(prompt, sizeof(prompt), "enseash [exit:%d|%ldms] %% ", WEXITSTATUS(status), ms);
                    else if (WIFSIGNALED(status))
                        snprintf(prompt, sizeof(prompt), "enseash [sign:%d|%ldms] %% ", WTERMSIG(status), ms);
                    
                    remove_job(z_pid);
                    break;
                }
            }
        }
        // --------------------------------------------------

        write(STDOUT_FILENO, prompt, strlen(prompt));
        ssize_t bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE);

        if (bytes_read <= 0) break;
        buffer[bytes_read - 1] = '\0';
        if (strncmp(buffer, "exit", 4) == 0 && bytes_read == 5) break;

        // On garde une copie propre de la commande pour l'affichage background
        strncpy(raw_command, buffer, BUFFER_SIZE);

        argv[0] = strtok(buffer, " \t");
        if (argv[0] == NULL) continue;
        int i = 0;
        while (argv[i] != NULL && i < MAX_ARGS - 1) {
            i++;
            argv[i] = strtok(NULL, " \t");
        }

        // <--- Q9: DÉTECTION DU BACKGROUND (&)
        int is_bg = 0;
        // On regarde le dernier argument
        int last_arg_idx = i - 1;
        if (last_arg_idx >= 0 && strcmp(argv[last_arg_idx], "&") == 0) {
            is_bg = 1;
            argv[last_arg_idx] = NULL; // On retire le '&' pour pas que execvp le lise
        }

        // Recherche PIPE (comme Q8)
        int pipe_index = -1;
        for (int k = 0; argv[k] != NULL; k++) {
            if (strncmp(argv[k], "|", 1) == 0) { pipe_index = k; break; }
        }

        clock_gettime(CLOCK_REALTIME, &start); // Start chrono

        if (pipe_index != -1) {
            // CAS PIPE (On ne gère pas le background sur les pipes pour simplifier ici, 
            // mais ce serait possible)
            argv[pipe_index] = NULL;
            char **argv2 = &argv[pipe_index + 1];
            int pipefd[2]; pipe(pipefd);
            pid_t p1 = exec_command(argv, STDIN_FILENO, pipefd[1]); close(pipefd[1]);
            pid_t p2 = exec_command(argv2, pipefd[0], STDOUT_FILENO); close(pipefd[0]);
            waitpid(p1, &status, 0);
            waitpid(p2, &status, 0);
            // Prompt normal pour pipe
            clock_gettime(CLOCK_REALTIME, &end);
            long ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
            snprintf(prompt, sizeof(prompt), "enseash [exit:%d|%ldms] %% ", WEXITSTATUS(status), ms);
        } 
        else {
            // CAS NORMAL (Avec gestion Background)
            pid_t pid = exec_command(argv, STDIN_FILENO, STDOUT_FILENO);

            if (is_bg) {
                // <--- Q9: CAS BACKGROUND
                // On n'attend PAS (pas de wait bloquant)
                // On ajoute à la liste
                add_job(pid, raw_command, start);
                
                // Le prompt change pour indiquer qu'on a lancé un job
                snprintf(prompt, sizeof(prompt), "enseash [%d&] %% ", jobs_count);
            } 
            else {
                // <--- CAS FOREGROUND (Classique)
                waitpid(pid, &status, 0);
                
                clock_gettime(CLOCK_REALTIME, &end);
                long ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
                
                if (WIFEXITED(status))
                    snprintf(prompt, sizeof(prompt), "enseash [exit:%d|%ldms] %% ", WEXITSTATUS(status), ms);
                else if (WIFSIGNALED(status))
                    snprintf(prompt, sizeof(prompt), "enseash [sign:%d|%ldms] %% ", WTERMSIG(status), ms);
            }
        }
    }
    
    write(STDOUT_FILENO, "Bye bye...\n", 11);
    return 0;
}