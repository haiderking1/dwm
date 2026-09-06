#include "test.h"

int
main(void)
{
    test_commands();
    test_memory();
    test_state();
    test_transport();
    puts("bar-bridge: command, memory, state, and X11 transport tests passed");
    return 0;
}
