#include "parser.h" 
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int get_out(struct Command *cmd) {
    cmd->out_exists = true;
    char ch;
    bool started = false;
    cmd->out = malloc(sizeof(char)*8);
    int out_size = 8;
    int chars = 0;
    memset(cmd->out,0,sizeof(out_size));

    while (true) {
        ch = getchar();
        if (ch == '>') continue;
        if (ch == ' ') {
            if (started) return 0;
            continue;
        }
        else if (ch == '\n') return 1;
        started = true;
        if (chars >= out_size-1) 
            resize_array((void**)&cmd->out,&out_size,sizeof(char));
        cmd->out[chars++] = ch;
    }
}

int get_in(struct Command *cmd) {
    cmd->in_exists = true;
    char ch;
    bool started = false;
    cmd->in = malloc(sizeof(char)*8);
    int in_size = 8;
    int chars = 0;
    memset(cmd->in,0,sizeof(in_size));

    while (true) {
        ch = getchar();
        if (ch == '<') continue;
        if (ch == ' ') {
            if (started) return 0;
            continue;
        }
        else if (ch == '\n') return 1;
        started = true;
        if (chars >= in_size-1) 
            resize_array((void**)&cmd->in,&in_size,sizeof(char));
        cmd->in[chars++] = ch;
    }
}

void get_string(struct Command *cmd) {
    ++cmd->argc;
    if (cmd->argc > cmd->argv_size) 
        resize_array((void**)&cmd->argv,&cmd->argv_size,sizeof(char*));
    char ch;
    bool ignore = false;
    int i = 0;
    int argument_size = 0;
    char **argument = &cmd->argv[cmd->argc-1];

    while (true) {
        ch = getchar();
        if (ignore) {
            ignore = false;
            goto put_element;
        }
        if (ch == '\"') return;
        if (ch == '\\') ignore = true;

put_element:
        if (i >= argument_size) {
            resize_array((void**)argument,&argument_size,sizeof(char));
        }
        (*argument)[i++] = ch;
    }
}

int get_argument(struct Command *cmd, char ch) {
    ++cmd->argc;
    if (cmd->argc > cmd->argv_size) 
        resize_array((void**)&cmd->argv,&cmd->argv_size,sizeof(char*));
    cmd->argv[cmd->argc-1] = malloc(2*sizeof(char));
    memset(cmd->argv[cmd->argc-1],0,2*sizeof(char));
    cmd->argv[cmd->argc-1][0] = ch;
    int i = 1;
    int argument_size = 2;

    while (true) {
        ch = getchar();
        if (ch == ' ') return 0;
        else if (ch == '\n') return 1;
        if (i >= argument_size) {
            resize_array((void**)&cmd->argv[cmd->argc-1],&argument_size,sizeof(char));
        }
        cmd->argv[cmd->argc-1][i++] = ch;
    }
}

int get_command(struct Command *cmd) {
    char ch;
    bool started = false;
    cmd->name = malloc(sizeof(char)*8);
    memset(cmd->name,0,sizeof(cmd->name));
    int name_size = 8;
    int chars = 0;

    while (true) {
        ch = getchar();
        if (ch == ' ') {
            if (started) return 0;
            continue;
        }
        else if (ch == '\n') return 1;
        started = true;
        if (chars >= name_size-1)
            resize_array((void**)&cmd->name,&name_size,sizeof(char));
        cmd->name[chars++] = ch;
    }
    return 0;
}

int parse_single_command(struct Command *stream, int n) { 
    struct Command cmd;
    cmd.argc = 0;
    cmd.in_exists = false;
    cmd.out_exists = false;
    cmd.argv_size = 0;

    if (get_command(&cmd)) goto exit;

    while (true) {
        char ch;
        ch = getchar();
        switch (ch) {
            case ' ': continue;
            case '|':
                stream[n-1] = cmd;
                return 1;
            case '\n':
                goto exit;
            case '\"':
                get_string(&cmd);
                break;
            case '<':
                if (get_in(&cmd)) goto exit;
                break;
            case '>':
                if (get_out(&cmd)) goto exit;
                break;
            default:
                if (get_argument(&cmd,ch)) goto exit;
                break;
        }
    }
exit:
    stream[n-1] = cmd;
    return 0;
}

void print_command(struct Command *stream, int n) {
    for (int i = 0; i < n; ++i) {
        printf("%d'th command is: %s\n",i+1,stream[i].name);
        for (int j = 0; j < stream[i].argc; ++j) {
            printf("%d'th argument is: %s\n",j+1,stream[i].argv[j]);
        }
        if (stream[i].in_exists) {
            printf("Input source is: %s\n",stream[i].in);
        }
        if (stream[i].out_exists) {
            printf("Output destination is: %s\n",stream[i].out);
        }
    }
}

void clean_up(struct Command *command_stream, int commands_num) {
    for (int i = 0; i < commands_num; ++i) {
        struct Command cmd = command_stream[i];
        for (int j = 0; j < cmd.argc; ++j) {
            free(cmd.argv[j]);
        }
        if (cmd.in_exists) free(cmd.in);
        if (cmd.out_exists) free(cmd.out);
        free(cmd.name);
    } 
    free(command_stream);
}

void parse_commands(struct Command **command_stream, int *commands_num) {
    *command_stream = malloc(sizeof(struct Command)); 
    *commands_num = 1;
    int stream_size = 1;

    while (true) {
        if (!parse_single_command(*command_stream,(*commands_num)++)) break;   
        if (*commands_num >= stream_size) {
            resize_array((void**)command_stream,&stream_size,sizeof(struct Command));
        } 
    }
    --*commands_num;
    print_command(*command_stream,*commands_num);
}