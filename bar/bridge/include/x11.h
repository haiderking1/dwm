#ifndef BRIDGE_X11_H
#define BRIDGE_X11_H
#include <X11/Xlib.h>
#include "command.h"

int bridge_send_command(Display *display, Window root, Atom atom,
                        const struct bridge_command *command);
int bridge_watch(Display *display, Window root);
#endif
