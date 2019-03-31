#include "parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "fcntl.h"

int main() {
    while (true) {
        printf("> ");
        struct Command *command_stream;
        int commands_num;
        parse_commands(&command_stream,&commands_num);
        pid_t *pid_running = malloc(commands_num*sizeof(int));
        int prev_fd = STDIN_FILENO;

        for (int i = 0; i < commands_num; ++i) {
            int fd[2];
            if (pipe(fd) == -1) {
                printf("Couldn't create pipe!\n");
                break;
            }
            pid_t pid = fork();
            if (pid == 0) { // if child 
                if (i == 0 && command_stream[i].in_exists) {
                    int fdinp = open(command_stream[i].in,O_RDONLY);
                    if (fdinp == -1) {
                        printf("Unable to open input file!\n");
                        break;
                    }
                    prev_fd = fdinp;
                }
                if (i == commands_num - 1 && command_stream[i].out_exists) {
                    int fdout = open(command_stream[i].out,O_WRONLY|O_CREAT,0644);
                    if (fdout == -1) {
                        printf("Unable to open output file!\n");
                        break;
                    }
                    dup2(fdout,STDOUT_FILENO);
                }
                dup2(prev_fd,STDIN_FILENO);
                if (i != commands_num - 1) dup2(fd[1],STDOUT_FILENO);
                close(fd[0]);
                execvp(command_stream[i].name,command_stream[i].argv);
            } else if (pid != -1) { // parent
                pid_running[i] = pid;
                prev_fd = fd[0];
                close(fd[1]);
            } else {
                printf("Couldn't fork!\n");
                break;
            }
        }

        for (int i = 0; i < commands_num; ++i) {
            int status;
            waitpid(pid_running[i],&status,0);
        }

        clean_up(command_stream,commands_num);
        free(pid_running);
    }
    return 0;
}