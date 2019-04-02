#ifndef __parser__
#define __parser__

#include <stdbool.h>

enum QStatus {
    DEFAULT,
    PIPE,
    AND,
    OR,
    BG,
};

struct Command {
    char *name;
    char **argv;
    int argc;
    char *in;
    char *out;
    enum QStatus status;
    bool output_rewrite;
};

void parse_commands(struct Command **command_stream, int *commands_num);
void clean_up(struct Command *command_stream, int commands_num);

#endif