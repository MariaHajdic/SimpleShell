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
    int *last_pid_idx, 
    pid_t pid
) {
    close(fd[1]);
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

void wait_bg() {
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0) {}
}

int main() {
    while (true) {
        // printf("> ");
        struct CommandStream stream = parse_commands();
        
        pid_t *pid_running = malloc(stream.size * sizeof(pid_t));
        int prev_fd = STDIN_FILENO;
        int last_pid_idx = -1;
        bool prev_success = true;
        bool has_bg_shell = false;
        bool is_bg_shell = false;

        if (stream.bg) {
            pid_t sh_pid = fork();
            if (sh_pid == 0) {
                is_bg_shell = true;
            } else {
                has_bg_shell = true;
                stream.size = 0;
             }
        }

        for (int i = 0; i < stream.size; ++i) {
            struct Command cmd = stream.cmds[i];

            bool exec_needed = true;
            if (i > 0 && stream.cmds[i - 1].status == AND) {
                exec_needed = prev_success;
            }
            if (i > 0 && stream.cmds[i - 1].status == OR) {
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
            bool allow_input_redirect = (i == 0 || stream.cmds[i-1].status != PIPE);
            if (pid == 0) { // if child 
                return exec_child(&cmd, prev_fd, fd, allow_input_redirect);
            } else if (pid != -1) { // parent
                prev_success = exec_parent(&cmd, &prev_fd, fd, pid_running, 
                    &last_pid_idx, pid);
            } else {
                printf("Couldn't fork!\n");
                break;
            }
        }

        if (!has_bg_shell) {
            for (int i = 0; i <= last_pid_idx; ++i) {
                int status;
                waitpid(pid_running[i], &status, 0);
            }
        }
        if (is_bg_shell) {
            exit(0);
        }

        wait_bg();
        clean_up(&stream);
        free(pid_running);
    }
    return 0;
}