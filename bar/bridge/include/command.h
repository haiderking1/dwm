#ifndef BRIDGE_COMMAND_H
#define BRIDGE_COMMAND_H

enum bridge_mode { BRIDGE_WATCH, BRIDGE_COMMAND };
struct bridge_command {
    enum bridge_mode mode;
    int operation;
    int monitor;
    unsigned int argument;
};

/* Parse the complete argv, including the executable name. */
int bridge_parse_args(int argc, char *const argv[], struct bridge_command *out);
#endif
