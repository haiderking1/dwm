#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xatom.h>
#include "xmock.h"
QsMock mock;

int XGetWindowAttributes(Display *display, Window window, XWindowAttributes *wa)
{
 (void)display;
 if (window != mock.window) return 0;
 memset(wa, 0, sizeof *wa);
 wa->override_redirect = mock.override_redirect;
 wa->map_state = mock.mapped;
 wa->your_event_mask = mock.selected_events;
 return 1;
}

int XSelectInput(Display *display, Window window, long mask)
{
 (void)display; assert(window == mock.window);
 mock.selected_events = mask;
 return 1;
}

int XGetWindowProperty(Display *display, Window window, Atom property,
 long offset, long length, Bool remove, Atom requested, Atom *actual,
 int *format, unsigned long *count, unsigned long *extra, unsigned char **data)
{
 unsigned long *values = NULL, atom = mock.dock_atom;
 int n = 0;
 (void)display; (void)offset; (void)remove;
 assert(window == mock.window);
 *actual = None; *format = 0; *count = *extra = 0; *data = NULL;
 if (property == mock.type_atom && mock.dock) { values = &atom; n = 1; }
 if (property == mock.partial_atom) { values = mock.partial; n = mock.partial_count; }
 if (property == mock.strut_atom) { values = mock.legacy; n = mock.legacy_count; }
 if (n) {
  *actual = requested; *format = 32;
  *count = n < length ? n : length;
  *extra = (n - *count) * 4;
  *data = malloc(*count * sizeof *values);
  assert(*data); memcpy(*data, values, *count * sizeof *values);
 }
 return Success;
}

int XMapWindow(Display *display, Window window)
{
 (void)display; assert(window == mock.window);
 mock.mapped = IsViewable; mock.maps++;
 return 1;
}

int XChangeProperty(Display *display, Window window, Atom property, Atom type,
 int format, int mode, const unsigned char *data, int length)
{
 (void)display;
 assert(window == mock.root && property == mock.state_atom);
 assert(type == mock.utf8_atom && format == 8 && mode == PropModeReplace);
 free(mock.state); mock.state = malloc(length + 1); assert(mock.state);
 memcpy(mock.state, data, length); mock.state[length] = 0;
 mock.property_writes++;
 return 1;
}

int XDeleteProperty(Display *display, Window window, Atom property)
{
 (void)display; assert(window == mock.root && (property == mock.state_atom || property == 9));
 return 1;
}

int XFlush(Display *display) { (void)display; return 1; }

int XMoveResizeWindow(Display *display, Window window, int x, int y,
                      unsigned int width, unsigned int height)
{
 (void)display; (void)x;
 assert(window == 100 && y < 0 && width > 0 && height > 0);
 mock.compatibility_moves++;
 return 1;
}

int XSetInputFocus(Display *display, Window window, int revert, Time time)
{
 (void)display; (void)revert; (void)time; assert(window == mock.root);
 return 1;
}
