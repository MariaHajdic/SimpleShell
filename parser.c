#include "parser.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>

void skip_whitespaces() {
    char ch = getchar();
    while (ch == ' ') {
        ch = getchar();
    }
    ungetc(ch, stdin);
}

void skip_comment() {
    char ch = getchar();
    while (ch != '\n') {};
    ungetc(ch, stdin);
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

void print_command(struct Command *stream, int n) {
    for (int i = 0; i < n; ++i) {
        printf("%d'th command is: %s\n", i+1, stream[i].name);
        for (int j = 1; j < stream[i].argc; ++j) {
            printf("%d'th argument is: %s\n", j, stream[i].argv[j]);
        }
        if (stream[i].in != NULL) {
            printf("Input source is: %s\n", stream[i].in);
        }
        if (stream[i].out != NULL) {
            printf("Output destination is: %s\n", stream[i].out);
        }
    }
}

void get_token(char **token, char ending) {
    if (*token == NULL) {
        *token = calloc(2 * sizeof(char), 0);
    }
    int size = 2;
    int i = 0;
    bool ignore = false;
    
    while (true) {
        char ch = getchar();
        if (ignore) {
            ignore = false;
            if (ch == '\n') 
                continue;
        } else {
            if (ch == '\n') {
                ungetc(ch, stdin);
                return;
            }
            if (ch == ending) 
                return;
            if (ch == '\\') {
                ignore = true;
                continue;
            }
        }
        if (i >= size - 1) {
            resize_array(token, &size, i + 1);
        }
        (*token)[i++] = ch;
    }   
}

void put_argument(struct Command *cmd, char ending) {
    cmd->argv = realloc(cmd->argv, (cmd->argc + 2) * sizeof(char*));
    cmd->argv[cmd->argc + 1] = NULL;
    get_token(&cmd->argv[cmd->argc++], ending);
}

/* returns true if waiting for another command, else false */
bool parse_single_command(struct Command **stream, int *cnum) {
    struct Command cmd;
    cmd.name = NULL;
    cmd.argv = NULL;
    cmd.argc = 0;
    cmd.in = NULL;
    cmd.out = NULL;
    cmd.status = DEFAULT;
    cmd.output_rewrite = true;

    skip_whitespaces();
    char chr = getchar();
    if (chr == '#')
        skip_comment();
    else {
        ungetc(chr, stdin);
        get_token(&cmd.name, ' ');
        cmd.argv = malloc(2 * sizeof(char*));
        cmd.argv[0] = cmd.name;
        cmd.argv[1] = NULL;
        cmd.argc = 1;
    }

    bool awaiting_next_cmd = true;

    while (true) {
        char ch = getchar();
        switch (ch) {
            case '#':
                skip_comment();
                break;
            case ' ':
                continue;
            case '\n':
                awaiting_next_cmd = false;
                goto exit;
            case '>':
                ch = getchar();
                if (ch == '>') 
                    cmd.output_rewrite = false;
                else 
                    ungetc(ch, stdin);
                skip_whitespaces();
                get_token(&cmd.out, ' ');
                break;
            case '<':
                skip_whitespaces();
                get_token(&cmd.in, ' ');
                break;
            case '|':
                ch = getchar();
                if (ch == '|') 
                    cmd.status = OR;
                else {
                    cmd.status = PIPE;
                    ungetc(ch, stdin);
                }
                goto exit;
            case '&':
                ch = getchar();
                if (ch == '&') 
                    cmd.status = AND;
                else {
                    cmd.status = BG;
                    ungetc(ch, stdin);
                }
                goto exit;
            case '\"':
                put_argument(&cmd, '\"');
                break;
            case '\'':
                put_argument(&cmd, '\'');
                break;
            default:
                ungetc(ch, stdin);
                put_argument(&cmd, ' ');
                break;
        }
    }

exit:
    if (cmd.name == NULL) 
        return false;
    *stream = realloc(*stream, ((*cnum) + 1) * sizeof(struct Command));
    (*stream)[(*cnum)++] = cmd;
    return awaiting_next_cmd; 
}

void parse_commands(struct Command **stream, int *cnum) {
    while (parse_single_command(stream, cnum)) {};
}