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

int exec_child(struct Command *cmd, int prev_fd, int *fd, bool allow_input_redirect) {
    if (allow_input_redirect && cmd->in_exists ) {
        int fdinp = open(cmd->in,O_RDONLY);
        if (fdinp == -1) {
            printf("Unable to open input file!\n");
            return 1;
        }
        prev_fd = fdinp;
    }
    if (cmd->status != PIPE && cmd->out_exists) {
        int fdout = open(cmd->out,O_WRONLY|O_CREAT,0644);
        if (fdout == -1) {
            printf("Unable to open output file!\n");
            return 1;
        }
        dup2(fdout,STDOUT_FILENO);
    }
    dup2(prev_fd,STDIN_FILENO);
    if (cmd->status == PIPE) dup2(fd[1],STDOUT_FILENO);
    close(fd[0]);
    execvp(cmd->name,cmd->argv);
    return 0;
}

int exec_parent(struct Command *cmd, int *prev_fd, int *fd, int *last_andor, int i, pid_t *pid_running, pid_t *pid) {
    *prev_fd = fd[0];
    close(fd[1]);
    if (cmd->status == AND || cmd->status == OR) {
        int status;
        for (int j = *last_andor; j <= i; ++j) {
            waitpid(pid_running[j],&status,0);
        }
        *last_andor = i + 1;
        if ((cmd->status == AND && status != 0) || (cmd->status == OR && status == 0)) {
            return 1;
        }
    } else pid_running[i] = *pid;
    return 0;
}

int main() {
    while (true) {
        printf("> ");
        struct Command *command_stream;
        int commands_num;
        parse_commands(&command_stream,&commands_num);
        pid_t *pid_running = malloc(commands_num*sizeof(int));
        int prev_fd = STDIN_FILENO;

        int last_andor = 0;
        bool exec_the_rest = true;
        for (int i = 0; i < commands_num; ++i) {
            int fd[2];
            if (pipe(fd) == -1) {
                printf("Couldn't create pipe!\n");
                break;
            }
            pid_t pid = fork();
            bool allow_input_redirect = (i == 0 || command_stream[i-1].status != PIPE);
            if (pid == 0 && exec_child(&command_stream[i],prev_fd,fd,allow_input_redirect)) { // if child 
                exec_the_rest = false;
                break;
            } else if (pid != -1) { // parent
                if (exec_parent(&command_stream[i],&prev_fd,fd,&last_andor,i,pid_running,&pid)) {
                    exec_the_rest = false;
                    break;
                }
            } else {
                exec_the_rest = false;
                printf("Couldn't fork!\n");
                break;
            }
        }
        if (exec_the_rest) {
            for (int i = last_andor; i < commands_num; ++i) {
                int status;
                waitpid(pid_running[i],&status,0);
            }
        }
        clean_up(command_stream,commands_num);
        free(pid_running);
    }
    return 0;
}