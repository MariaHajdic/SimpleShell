#include "parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int main() {
    while (true) {
        struct Command *command_stream;
        int commands_num;
        parse_commands(&command_stream,&commands_num);
        pid_t *pid_running = malloc(commands_num*sizeof(int));

        for (int i = 0; i < commands_num; ++i) {
            pid_t pid = fork();
            if (pid == 0) { // if child 
                execvp(command_stream[i].name,command_stream[i].argv); // p for path
            } else if (pid != -1) { // parent
                pid_running[i] = pid;
            } else {
                printf("Couldn't fork!\n");
                return 1;
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