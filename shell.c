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
        int flags = O_WRONLY | O_CREAT;
        if (cmd->output_rewrite) 
            flags |= O_TRUNC;
        else 
            flags |= O_APPEND;
        int fdout = open(cmd->out, flags, 0644);
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

bool exec_parent(
    struct Command *cmd, 
    int *prev_fd, 
    int *fd, 
    pid_t *pid_running, 
    pid_t **pid_bg,
    int *bg_size,
    int *last_pid_idx, 
    pid_t pid
) {
    close(fd[1]);
    if (cmd->status == BG) {
        *pid_bg = realloc(*pid_bg, (*bg_size + 1) * sizeof(pid_t));
        (*pid_bg)[(*bg_size)++] = pid;
        return true;
    }
    pid_running[++*last_pid_idx] = pid;
    if (cmd->status == AND || cmd->status == OR) {
        int status;
        for (int j = 0; j <= *last_pid_idx; ++j) {
            waitpid(pid_running[j], &status, 0);
        }
        *last_pid_idx = -1;
        return status == 0;
    }
    *prev_fd = fd[0];
    return true;
}

void wait_bg(pid_t **pid_bg, int* bg_size) {
    pid_t *tmp = malloc(*bg_size * sizeof(pid_t));
    int j = 0;
    int new_bg_size = 0;
    for (int i = 0; i < *bg_size; ++i) {
        int status;
        if (waitpid((*pid_bg)[i], &status, WNOHANG) == 0) {
            tmp[j++] = (*pid_bg)[i];
            ++new_bg_size;
        }
    }
    *pid_bg = realloc(*pid_bg, new_bg_size * sizeof(pid_t));
    memcpy(*pid_bg, tmp, new_bg_size * sizeof(pid_t));
    *bg_size = new_bg_size;
    free(tmp);
}

int main() {
    pid_t *pid_bg = NULL;
    int bg_size = 0;

    while (true) {
        printf("> ");
        struct Command *command_stream = NULL;
        int commands_num = 0;
        parse_commands(&command_stream, &commands_num);
        pid_t *pid_running = malloc(commands_num * sizeof(pid_t));
        int prev_fd = STDIN_FILENO;
        int last_pid_idx = -1;
        bool prev_success = true;

        for (int i = 0; i < commands_num; ++i) {
            struct Command cmd = command_stream[i];

            bool exec_needed = true;
            if (i > 0 && command_stream[i - 1].status == AND) {
                exec_needed = prev_success;
            }
            if (i > 0 && command_stream[i - 1].status == OR) {
                exec_needed = !prev_success;
            }
            if (!exec_needed)
                continue;


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
                prev_success = exec_parent(&cmd, &prev_fd, fd, pid_running, 
                    &pid_bg, &bg_size, &last_pid_idx, pid);
            } else {
                printf("Couldn't fork!\n");
                break;
            }
        }
        for (int i = 0; i <= last_pid_idx; ++i) {
            int status;
            waitpid(pid_running[i], &status, 0);
        }
        wait_bg(&pid_bg, &bg_size);

        clean_up(command_stream, commands_num);
        free(pid_running);
    }
    if (pid_bg != NULL) 
        free(pid_bg);
    return 0;
}