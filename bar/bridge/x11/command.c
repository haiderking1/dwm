#include "x11.h"

#include <string.h>

int
bridge_send_command(Display *display, Window root, Atom atom,
                    const struct bridge_command *command)
{
    XEvent event;
    int sent;

    memset(&event, 0, sizeof event);
    event.xclient.type = ClientMessage;
    event.xclient.display = display;
    event.xclient.window = root;
    event.xclient.message_type = atom;
    event.xclient.format = 32;
    event.xclient.data.l[0] = command->operation;
    event.xclient.data.l[1] = command->monitor;
    event.xclient.data.l[2] = command->argument;
    sent = XSendEvent(display, root, False,
                     SubstructureRedirectMask | SubstructureNotifyMask, &event);
    XSync(display, False);
    return sent != 0;
}
