#include "parser.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void skip_whitespaces() {
    char ch = getchar();
    while (ch == ' ' || ch == '\t') {
        ch = getchar();
    }
    ungetc(ch, stdin);
}

void skip_comment() {
    char ch = getchar();
    while (ch != '\n' && ch != EOF) {
        ch = getchar();
    }
    ungetc(ch, stdin);
}

void clean_up(struct CommandStream *stream) {
    for (int i = 0; i < stream->size; ++i) {
        struct Command cmd = stream->cmds[i];
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
    free(stream->cmds);
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
        *token = calloc(2, sizeof(char));
    }
    int size = 2;
    int i = 0;
    bool ignore = false;
    bool ignore_new_line = false;

    if (ending == '\'' || ending == '\"') 
        ignore_new_line = true;
    
    while (true) {
        char ch = getchar();
        if (ignore) {
            ignore = false;
            if (ch == '\n') 
                continue;
        } else {
            if (ch == EOF) 
                return;
            if (ending == ' ' && ch == '|') {
                ungetc(ch, stdin);
                return;
            }
            if (ch == '\n' && !ignore_new_line) {
                ungetc(ch, stdin);
                return;
            }
            if (ending == ' ' && ch == '\t')
                return;
            if (ch == ending) 
                return;
            if (ch == '\\') {
                ignore = true;
                continue;
            }
        }
        if (i >= size - 1) {
            size = resize_array(token, size);
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
bool parse_single_command(struct CommandStream *stream) {
    struct Command cmd;
    memset(&cmd, 0, sizeof(cmd));
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
    bool ignore_new_line = false;

    while (true) {
        char ch = getchar();
        switch (ch) {
            case '#':
                skip_comment();
                break;
            case '\\':
                ignore_new_line = true;
                break;
            case '\t':
            case ' ':
                skip_whitespaces();
                break;
            case EOF:
                exit(0);
            case '\n':
                if (ignore_new_line) {
                    ignore_new_line = false;
                    break;
                }
                awaiting_next_cmd = false;
                goto exit;
            case '>':
                ch = getchar();
                if (ch == '>') 
                    cmd.output_rewrite = false;
                else 
                    ungetc(ch, stdin);
                skip_whitespaces();
                char end_char = ' ';
                ch = getchar();
                if (ch == '\"' || ch == '\'') {
                    end_char = ch;
                } else {
                    ungetc(ch, stdin);
                }
                get_token(&cmd.out, end_char);
                break;
            case '<':
                skip_whitespaces();
                end_char = ' ';
                ch = getchar();
                if (ch == '\"' || ch == '\'') {
                    end_char = ch;
                } else {
                    ungetc(ch, stdin);
                }
                get_token(&cmd.out, end_char);
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
                    stream->bg = true;
                    awaiting_next_cmd = false;
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
    stream->cmds = realloc(stream->cmds, (stream->size + 1) * sizeof(struct Command));
    stream->cmds[stream->size++] = cmd;
    return awaiting_next_cmd; 
}

struct CommandStream parse_commands() {
    struct CommandStream stream;
    memset(&stream, 0, sizeof(stream));
    while (parse_single_command(&stream)) {};
    return stream;
}