#include "command.h"

#include <limits.h>
#include <string.h>

static int
number(const char *text, unsigned int maximum, unsigned int *out)
{
    unsigned int value = 0;
    const unsigned char *p = (const unsigned char *)text;

    /* No signs, whitespace, radix prefixes, or trailing characters. */
    if (!*p)
        return 0;
    for (; *p; ++p) {
        unsigned int digit;
        if (*p < '0' || *p > '9')
            return 0;
        digit = *p - '0';
        if (value > maximum / 10 ||
            (value == maximum / 10 && digit > maximum % 10))
            return 0;
        value = value * 10 + digit;
    }
    *out = value;
    return 1;
}

int
bridge_parse_args(int argc, char *const argv[], struct bridge_command *out)
{
    struct bridge_command command = { BRIDGE_COMMAND, 0, 0, 0 };
    unsigned int monitor, tag;

    if (argc == 2 && strcmp(argv[1], "--watch") == 0) {
        command.mode = BRIDGE_WATCH;
    } else {
        if (argc < 3)
            return 0;
        if (strcmp(argv[1], "view") == 0)
            command.operation = 1;
        else if (strcmp(argv[1], "move") == 0)
            command.operation = 2;
        else if (strcmp(argv[1], "layout") == 0)
            command.operation = 3;
        else
            return 0;
        if (argc != (command.operation == 3 ? 3 : 4) ||
            !number(argv[2], INT_MAX, &monitor))
            return 0;
        command.monitor = (int)monitor;
        if (command.operation != 3) {
            if (!number(argv[3], 10, &tag) || tag == 0)
                return 0;
            command.argument = 1U << (tag - 1);
        }
    }
    *out = command;
    return 1;
}
