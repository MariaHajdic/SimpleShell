#ifndef __parser__
#define __parser__

#include <stdbool.h>

enum QStatus {
    DEFAULT,
    PIPE,
    AND,
    OR
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

struct CommandStream {
    int size;
    struct Command *cmds;
    bool bg;
};

struct CommandStream parse_commands();
void clean_up(struct CommandStream *stream);

#endif