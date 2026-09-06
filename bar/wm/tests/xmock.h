#ifndef QS_TEST_XMOCK_H
#define QS_TEST_XMOCK_H
#include <X11/Xlib.h>
typedef struct {
 Window window, root;
 Atom type_atom, dock_atom, partial_atom, strut_atom, state_atom, utf8_atom;
 int dock, mapped, override_redirect, partial_count, legacy_count;
 unsigned long partial[12], legacy[4];
 long selected_events;
 int property_writes, maps, compatibility_moves;
 char *state;
} QsMock;
extern QsMock mock;
#endif
