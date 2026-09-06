#include "test.h"
#include "command.h"

#include <limits.h>
#include <string.h>

void
test_commands(void)
{
    struct bridge_command command;
    char *args[] = { "bridge", "view", "0", "1", "extra" };
    char maximum[32], above[32], tag[8];
    const char *bad_monitors[] = {
        "", "-1", "+1", "-0", " 1", "1 ", "1\n", "1x", "0x1",
        "1.0", "99999999999999999999999999999999999999"
    };
    const char *bad_tags[] = {
        "", "0", "11", "-1", "+1", " 1", "1 ", "1\n", "1x",
        "0x1", "1.0", "99999999999999999999999999999999999999"
    };
    size_t i;
    int operation, n;

    snprintf(maximum, sizeof maximum, "%d", INT_MAX);
    snprintf(above, sizeof above, "%u", (unsigned int)INT_MAX + 1U);
    CHECK(!bridge_parse_args(0, args, &command));
    CHECK(!bridge_parse_args(1, args, &command));
    CHECK(!bridge_parse_args(2, args, &command));
    CHECK(!bridge_parse_args(3, args, &command));
    CHECK(!bridge_parse_args(5, args, &command));
    for (operation = 1; operation <= 2; ++operation) {
        args[1] = operation == 1 ? "view" : "move";
        for (n = 1; n <= 10; ++n) {
            snprintf(tag, sizeof tag, "%d", n);
            args[3] = tag;
            CHECK(bridge_parse_args(4, args, &command));
            CHECK(command.mode == BRIDGE_COMMAND);
            CHECK(command.operation == operation);
            CHECK(command.monitor == 0);
            CHECK(command.argument == (1U << (n - 1)));
        }
        args[2] = maximum;
        CHECK(bridge_parse_args(4, args, &command));
        CHECK(command.monitor == INT_MAX);
        args[2] = above;
        CHECK(!bridge_parse_args(4, args, &command));
        for (i = 0; i < sizeof bad_monitors / sizeof bad_monitors[0]; ++i) {
            args[2] = (char *)bad_monitors[i];
            CHECK(!bridge_parse_args(4, args, &command));
        }
        args[2] = "0";
        for (i = 0; i < sizeof bad_tags / sizeof bad_tags[0]; ++i) {
            args[3] = (char *)bad_tags[i];
            CHECK(!bridge_parse_args(4, args, &command));
        }
        args[3] = "1";
    }
    args[1] = "layout";
    CHECK(bridge_parse_args(3, args, &command));
    CHECK(command.operation == 3 && command.argument == 0);
    args[2] = maximum;
    CHECK(bridge_parse_args(3, args, &command));
    CHECK(command.monitor == INT_MAX);
    args[2] = above;
    CHECK(!bridge_parse_args(3, args, &command));
    for (i = 0; i < sizeof bad_monitors / sizeof bad_monitors[0]; ++i) {
        args[2] = (char *)bad_monitors[i];
        CHECK(!bridge_parse_args(3, args, &command));
    }
    args[2] = "0";
    CHECK(!bridge_parse_args(4, args, &command));
    args[1] = "--watch";
    CHECK(bridge_parse_args(2, args, &command));
    CHECK(command.mode == BRIDGE_WATCH);
    CHECK(!bridge_parse_args(3, args, &command));
    args[1] = "tag";
    CHECK(!bridge_parse_args(4, args, &command));
    args[1] = "VIEW";
    CHECK(!bridge_parse_args(4, args, &command));
    args[1] = "view";
    args[2] = "0001";
    args[3] = "010";
    CHECK(bridge_parse_args(4, args, &command));
    CHECK(command.monitor == 1 && command.argument == 512U);
}
