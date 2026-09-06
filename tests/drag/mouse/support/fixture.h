#ifndef DRAG_MOUSE_FIXTURE_H
#define DRAG_MOUSE_FIXTURE_H

/* Use Xlib's actual event unions, constants and function declarations. */
#include <X11/Xlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../../wm/bsp/bsp.h"

typedef struct Client Client;
typedef struct Monitor Monitor;
typedef struct { void (*arrange)(Monitor *); } Layout;
struct Client {
	Window win;
	int x, y, w, h, bw;
	float cfact;
	int isfloating, isfullscreen, oldstate;
	unsigned int tags;
	Client *next, *snext;
	Monitor *mon;
};
struct Monitor {
	Client *clients, *stack, *sel;
	unsigned int tagset[2], seltags, sellt;
	const Layout *lt[2];
	int num, wx, wy, ww, wh;
	int mx, slotx, sloty, slotw, sloth, step;
};

#define ISVISIBLE(C) ((C)->tags & (C)->mon->tagset[(C)->mon->seltags])
#define WIDTH(C) ((C)->w + 2 * (C)->bw)
#define HEIGHT(C) ((C)->h + 2 * (C)->bw)
#define MOUSEMASK (ButtonPressMask | ButtonReleaseMask | PointerMotionMask)
#define LIMIT 32
#define SOURCE ((Window)101)
#define TARGET ((Window)102)
#define DOCK_CHANGE ((Atom)201)
#define FLOAT_LAYOUT ((Atom)202)
#define FULLSCREEN_CHANGE ((Atom)203)

typedef struct { XEvent event; void (*before)(void); } QueuedEvent;
typedef struct { Window win; int x, y; } Move;
typedef struct { Window win; int x, y, alive; } ServerWindow;
typedef struct {
	Monitor monitors[2];
	Client *source, *target;
	Client source_before, target_before;
	ServerWindow windows[2];
	QueuedEvent queue[LIMIT];
	size_t queued, consumed;
	Move moves[LIMIT];
	size_t nmoves;
	int grabbed, grab_result, pointer_ok, startx, starty;
	int grabs, ungrabs, queries, raises, flushes, publishes, syncs;
	int enter_pending, enter_checks, updates, focuses, arrangements, resizes;
	int dispatches, dock_events, configure_replies;
	Window focused, destroyed;
	Monitor *arranged[LIMIT];
} Fixture;

static Fixture f;
static Display *dpy; /* Mocks accept NULL; no X connection is opened. */
static const Window root = 1;
enum { CurMove };
typedef struct { Cursor cursor; } TestCursor;
static TestCursor move_cursor = { 77 };
static TestCursor *cursor[] = { &move_cursor };
static const int refreshrate = 60;
static void (*handler[LASTEvent])(XEvent *);

/* Legacy mouse scenarios use a weighted layout and an empty BSP registry.
 * The real swap adapter still runs; BSP geometry is covered separately. */
#define TAGMASK 1023u
#define MAX(A,B) ((A) > (B) ? (A) : (B))
static void dwindle(Monitor *mon);
static void resizeclient(Client *, int, int, int, int);
static void tile(Monitor *mon);
static const Layout tiled_layout = { tile };
static const Layout floating_layout = { NULL };
static Client *wintoclient(Window window);
static void arrange(Monitor *mon);
static void dispatch_event(XEvent *event);

#endif
