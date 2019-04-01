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
    close(fd[0]);
    if (allow_input_redirect && cmd->in) {
        int fdinp = open(cmd->in,O_RDONLY);
        if (fdinp == -1) {
            printf("Unable to open input file!\n");
            return 1;
        }
        prev_fd = fdinp;
    }
    dup2(prev_fd, STDIN_FILENO);
    if (cmd->status != PIPE && cmd->out) {
        int fdout = open(cmd->out, O_WRONLY | O_CREAT, 0644);
        if (fdout == -1) {
            printf("Unable to open output file!\n");
            return 1;
        }
        dup2(fdout, STDOUT_FILENO);
    }
    if (cmd->status == PIPE) {
        dup2(fd[1], STDOUT_FILENO);
    }
    execvp(cmd->name, cmd->argv);
    return 0;
}

int exec_parent(
    struct Command *cmd, 
    int *prev_fd, 
    int *fd, 
    pid_t *pid_running, 
    int *last_pid_idx, 
    pid_t pid
) {
    close(fd[1]);
    if (cmd->status == AND || cmd->status == OR) {
        int status;
        for (int j = 0; j <= *last_pid_idx; ++j) {
            waitpid(pid_running[j], &status, 0);
        }
        *last_pid_idx = -1;
        if ((cmd->status == AND && status != 0) || (cmd->status == OR && status == 0)) {
            return 1;
        }
    } else {
        pid_running[++*last_pid_idx] = pid;
    }
    *prev_fd = fd[0];
    return 0;
}

int main() {
    while (true) {
        printf("> ");
        struct Command *command_stream;
        int commands_num;
        parse_commands(&command_stream, &commands_num);
        pid_t *pid_running = malloc(commands_num * sizeof(pid_t));
        int prev_fd = STDIN_FILENO;
        int last_pid_idx = -1;

        for (int i = 0; i < commands_num; ++i) {
            struct Command cmd = command_stream[i];
            if (!strcmp(cmd.name,"cd")) {
                chdir(cmd.argv[1]);
                continue;
            }
            int fd[2];
            if (pipe(fd) == -1) {
                printf("Couldn't create pipe!\n");
                break;
            }
            pid_t pid = fork();
            bool allow_input_redirect = (i == 0 || command_stream[i-1].status != PIPE);
            if (pid == 0) { // if child 
                return exec_child(&cmd, prev_fd, fd, allow_input_redirect);
            } else if (pid != -1) { // parent
                if (exec_parent(&cmd, &prev_fd, fd, pid_running, &last_pid_idx, pid)) {
                    break;
                }
            } else {
                printf("Couldn't fork!\n");
                break;
            }
        }
        for (int i = 0; i <= last_pid_idx; ++i) {
            int status;
            waitpid(pid_running[i], &status, 0);
        }
        clean_up(command_stream, commands_num);
        free(pid_running);
    }
    return 0;
}