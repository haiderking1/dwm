#include "command.h"
#include "x11.h"

#include <signal.h>
#include <stdio.h>

static int x_failed;

static int
x_error(Display *display, XErrorEvent *event)
{
    char message[256];
    x_failed = 1;
    XGetErrorText(display, event->error_code, message, sizeof message);
    fprintf(stderr, "dwm-bar-bridge: X11 error: %s\n", message);
    return 0;
}

int
main(int argc, char *argv[])
{
    struct bridge_command command;
    Display *display;
    int ok;
    struct sigaction action = {0};

    if (!bridge_parse_args(argc, argv, &command)) {
        fprintf(stderr, "usage: %s --watch\n"
                        "       %s view MONITOR TAG\n"
                        "       %s move MONITOR TAG\n"
                        "       %s layout MONITOR\n"
                        "MONITOR: 0..INT_MAX; TAG: 1..10\n",
                        argv[0], argv[0], argv[0], argv[0]);
        return 2;
    }
    action.sa_handler = SIG_IGN;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGPIPE, &action, NULL) != 0) {
        perror("dwm-bar-bridge: sigaction");
        return 1;
    }
    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "dwm-bar-bridge: cannot open X display\n");
        return 1;
    }
    XSetErrorHandler(x_error);
    if (command.mode == BRIDGE_WATCH)
        ok = bridge_watch(display, DefaultRootWindow(display));
    else
        ok = bridge_send_command(display, DefaultRootWindow(display),
                XInternAtom(display, "_DWM_QUICKSHELL_COMMAND", False), &command);
    XCloseDisplay(display);
    return ok && !x_failed ? 0 : 1;
}
