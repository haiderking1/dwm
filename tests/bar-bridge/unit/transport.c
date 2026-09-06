#include "test.h"
#include "x11.h"

#include <limits.h>

static XEvent captured;
static int sends, syncs;
static Status send_result = 1;

/* These replace Xlib calls. No X server is opened or contacted by tests. */
Status
XSendEvent(Display *display, Window window, Bool propagate, long mask, XEvent *event)
{
    CHECK(display == NULL);
    CHECK(window == 123);
    CHECK(propagate == False);
    CHECK(mask == (SubstructureRedirectMask | SubstructureNotifyMask));
    captured = *event;
    ++sends;
    return send_result;
}

int
XSync(Display *display, Bool discard)
{
    CHECK(display == NULL);
    CHECK(discard == False);
    CHECK(sends == syncs + 1);
    ++syncs;
    return 0;
}

void
test_transport(void)
{
    struct bridge_command command = { BRIDGE_COMMAND, 1, INT_MAX, 512U };
    int op;
    for (op = 1; op <= 3; ++op) {
        command.operation = op;
        command.argument = op == 3 ? 0 : 512U;
        CHECK(bridge_send_command(NULL, 123, 456, &command));
        CHECK(captured.xclient.type == ClientMessage);
        CHECK(captured.xclient.display == NULL);
        CHECK(captured.xclient.window == 123);
        CHECK(captured.xclient.message_type == 456);
        CHECK(captured.xclient.format == 32);
        CHECK(captured.xclient.data.l[0] == op);
        CHECK(captured.xclient.data.l[1] == INT_MAX);
        CHECK(captured.xclient.data.l[2] == (long)command.argument);
        CHECK(captured.xclient.data.l[3] == 0);
        CHECK(captured.xclient.data.l[4] == 0);
        CHECK(sends == syncs);
    }
    send_result = 0;
    CHECK(!bridge_send_command(NULL, 123, 456, &command));
    CHECK(sends == syncs);
}
