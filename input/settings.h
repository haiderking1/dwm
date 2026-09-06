#ifndef DWM_INPUT_SETTINGS_H
#define DWM_INPUT_SETTINGS_H

#include <X11/Xlib.h>

/*
 * Link settings.c with -lXi -lX11. Call setup after opening the display and
 * choosing its root. Call handle_event after XNextEvent, before normal dispatch.
 * Unrelated events and cookies are untouched. Matching cookies are acquired and
 * freed here, or borrowed without freeing if the caller already acquired them.
 *
 * One display, on the Xlib event-loop thread only. These functions temporarily
 * install a process-wide X error handler and restore the previous handler.
 * Settings persist on the server; there is no teardown or restoration.
 *
 * Relative physical/floating pointers include touchpads. Absolute-only devices
 * and master devices are not given driver property changes. Flat acceleration
 * is selected only when advertised; unsupported drivers retain their behavior
 * beyond the core 1/1, threshold-zero fallback. Core repeat is 200 ms / 29 ms.
 */
void input_setup(Display *display, Window root);
void input_handle_event(Display *display, XEvent *event);

#endif
