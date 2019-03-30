#include <stdbool.h>

struct Command {
    char *name;
    char **argv;
    int argc;
    int argv_size;
    bool in_exists;
    bool out_exists;
    char *in;
    char *out;
};

void parse_commands(struct Command **command_stream, int *commands_num);
void clean_up(struct Command *command_stream, int commands_num);