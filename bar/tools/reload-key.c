/* Invoke the existing dwm binding without terminating the session. */
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
int main(void) {
    Display *display = XOpenDisplay(NULL);
    XEvent event = {0};
    if (!display) { fputs("cannot open X display\n", stderr); return 1; }
    event.xkey.type = KeyPress;
    event.xkey.display = display;
    event.xkey.window = event.xkey.root = DefaultRootWindow(display);
    event.xkey.same_screen = True;
    event.xkey.state = Mod4Mask | ShiftMask;
    event.xkey.keycode = XKeysymToKeycode(display, XK_r);
    if (!XSendEvent(display, event.xkey.window, False, SubstructureRedirectMask, &event)) {
        XCloseDisplay(display); return 1;
    }
    XSync(display, False);
    XCloseDisplay(display);
    return 0;
}
