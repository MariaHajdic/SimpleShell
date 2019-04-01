#include "parser.h" 
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int get_out(struct Command *cmd) {
    char ch;
    bool started = false;
    int out_size = 0;
    int chars = 0;

    while (true) {
        ch = getchar();
        switch (ch) {
            case '>':
                continue;
            case ' ':
                if (started) {
                    return 0;
                }
                continue;
            case '\n':
                return 1;
        }
        started = true;
        if (chars >= out_size - 1) {
            resize_array((void**)&cmd->out, &out_size, sizeof(char), chars + 1);
        }
        cmd->out[chars++] = ch;
    }
}

int get_in(struct Command *cmd) {
    char ch;
    bool started = false;
    int in_size = 0;
    int chars = 0;

    while (true) {
        ch = getchar();
        switch (ch) {
            case '<':
                continue;
            case ' ':
                if (started) {
                    return 0;
                }
                continue;
            case '\n':
                return 1;
        }
        started = true;
        if (chars >= in_size - 1) {
            resize_array((void**)&cmd->in, &in_size, sizeof(char), chars + 1);
        }
        cmd->in[chars++] = ch;
    }
}

void get_string(struct Command *cmd, char quotes) {
    ++cmd->argc;
    if (cmd->argc >= cmd->argv_size - 1) {
        cmd->argv = realloc(cmd->argv, (cmd->argv_size + 1) * 2 * sizeof(char*));
        cmd->argv_size = (cmd->argv_size + 1) * 2;
    }
    cmd->argv[cmd->argc] = NULL;
        
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
        if (ch == '\"' && quotes == '\"') {
            return;
        }
        if (ch == '\'' && quotes == '\'') {
            return;
        }
        if (ch == '\\') {
            ignore = true;
        }

put_element:
        if (i >= argument_size - 1) {
            resize_array((void**)argument, &argument_size, sizeof(char), i + 1);
        }
        (*argument)[i++] = ch;
    }
}

int get_argument(struct Command *cmd, char ch) {
    ++cmd->argc;
    if (cmd->argc >= cmd->argv_size - 1) {
        cmd->argv = realloc(cmd->argv, (cmd->argv_size + 1) * 2 * sizeof(char*));
        cmd->argv_size = (cmd->argv_size + 1) * 2;
    }
    cmd->argv[cmd->argc] = NULL;
    cmd->argv[cmd->argc-1] = malloc(2 * sizeof(char));
    memset(cmd->argv[cmd->argc-1], 0, 2 * sizeof(char));
    cmd->argv[cmd->argc-1][0] = ch;
    int i = 1;
    int argument_size = 2;
    char **argument = &cmd->argv[cmd->argc - 1];

    while (true) {
        ch = getchar();
        if (ch == ' ') {
            return 0;
        } 
        if (ch == '\n') {
            return 1;
        }
        if (i >= argument_size - 1) {
            resize_array((void**)argument, &argument_size, sizeof(char), i + 1);
        }
        cmd->argv[cmd->argc-1][i++] = ch;
    }
}

int get_command(struct Command *cmd) {
    char ch;
    bool started = false;
    cmd->name = malloc(sizeof(char) * 8);
    memset(cmd->name, 0, sizeof(cmd->name));
    int name_size = 8;
    int chars = 0;
    int exit_code = 0;

    while (true) {
        ch = getchar();
        if (ch == ' ') {
            if (started) {
                goto put_cmd;
            }
            continue;
        }
        if (ch == '\n') {
            exit_code = 1;
            goto put_cmd;
        }
        started = true;
        if (chars >= name_size - 1) {
            resize_array((void**)&cmd->name, &name_size, sizeof(char), chars + 1);
        }
        cmd->name[chars++] = ch;
    }
put_cmd:
    cmd->argv_size = 2;
    cmd->argv = malloc(2 * sizeof(char*));
    cmd->argv[0] = cmd->name;
    cmd->argv[1] = NULL;
    cmd->argc = 1;
    return exit_code;
}

void skip_comment() {
    char ch;
    while (true) {
        ch = getchar();
        if (ch == '\n') return;
    }
}

int parse_single_command(struct Command *stream, int n) { 
    struct Command cmd;
    cmd.argc = 0;
    cmd.argv_size = 0;
    cmd.status = DEFAULT;
    cmd.in = NULL;
    cmd.out = NULL;

    if (get_command(&cmd)) goto exit;
    if (strlen(cmd.name) == 0) {
        return 1;
    }

    while (true) {
        char ch;
        char quotes;
        ch = getchar();
        switch (ch) {
            case ' ': continue;
            case '|':
                cmd.status = PIPE;
                ch = getchar();
                if (ch == '|') cmd.status = OR;
                stream[n-1] = cmd;
                return 0;
            case '&':
                cmd.status = BG;
                ch = getchar();
                if (ch == '&') cmd.status = AND;
                stream[n-1] = cmd;
                return 0;
            case '\n':
                goto exit;
            case '\'':
                quotes = '\'';
                get_string(&cmd,quotes);
                break;
            case '\"':
                quotes = '\"';
                get_string(&cmd,quotes);
                break;
            case '<':
                if (get_in(&cmd)) goto exit;
                break;
            case '>':
                if (get_out(&cmd)) goto exit;
                break;
            case '#':
                skip_comment();
                goto exit;
            default:
                if (get_argument(&cmd,ch)) goto exit;
                break;
        }
    }
exit:
    stream[n-1] = cmd;
    return 1;
}

void print_command(struct Command *stream, int n) {
    for (int i = 0; i < n; ++i) {
        printf("%d'th command is: %s\n", i+1, stream[i].name);
        for (int j = 1; j < stream[i].argc; ++j) {
            printf("%d'th argument is: %s\n", j, stream[i].argv[j]);
        }
        if (stream[i].in == NULL) {
            printf("Input source is: %s\n", stream[i].in);
        }
        if (stream[i].out == NULL) {
            printf("Output destination is: %s\n", stream[i].out);
        }
    }
}

void clean_up(struct Command *command_stream, int commands_num) {
    for (int i = 0; i < commands_num; ++i) {
        struct Command cmd = command_stream[i];
        for (int j = 1; j < cmd.argc; ++j) {
            free(cmd.argv[j]);
        }
        if (cmd.in) 
            free(cmd.in);
        if (cmd.out) 
            free(cmd.out);
        free(cmd.argv);
        free(cmd.name);
    } 
    free(command_stream);
}

void parse_commands(struct Command **command_stream, int *commands_num) {
    *command_stream = malloc(sizeof(struct Command)); 
    *commands_num = 1;
    int stream_size = 1;

    while (true) {
        if (parse_single_command(*command_stream, (*commands_num)++)) 
            break;   
        if (*commands_num >= stream_size) {
            resize_array((void**)command_stream, &stream_size, sizeof(struct Command), *commands_num);
        } 
    }
    --*commands_num;
    // print_command(*command_stream,*commands_num);
}