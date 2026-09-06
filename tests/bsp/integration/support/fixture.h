#ifndef BSP_INTEGRATION_FIXTURE_H
#define BSP_INTEGRATION_FIXTURE_H
#include <X11/Xlib.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../../wm/bsp/bsp.h"

typedef union { int i; float f; const void *v; } Arg;
typedef struct Client Client;
typedef struct Monitor Monitor;
typedef struct { void (*arrange)(Monitor *); } Layout;
struct Client {
	int oldx, oldy, oldw, oldh;
	Window win;
	int x, y, w, h, bw, isfloating, isfullscreen, oldstate;
	float cfact;
	unsigned int tags;
	Client *next, *snext;
	Monitor *mon;
};
struct Monitor {
	int num, wx, wy, ww, wh;
	float mfact;
	unsigned int tagset[2], seltags, sellt;
	const Layout *lt[2];
	Client *clients, *stack, *sel;
	Monitor *next;
};
#define MAX(A,B) ((A) > (B) ? (A) : (B))
#define TAGMASK 1023u
#define ISVISIBLE(C) ((C)->tags & (C)->mon->tagset[(C)->mon->seltags])
#define WIDTH(C) ((C)->w + 2 * (C)->bw)
#define HEIGHT(C) ((C)->h + 2 * (C)->bw)
#define MOUSEMASK (ButtonPressMask | ButtonReleaseMask | PointerMotionMask)
#define QUEUE_SIZE 64
static Monitor *mons, *selmon;
static Display *dpy;
static const Window root = 1;
enum { CurResize, CurMove };
typedef struct { Cursor cursor; } TestCursor;
static TestCursor resize_cursor = {77}, move_cursor = {78};
static TestCursor *cursor[] = { &resize_cursor, &move_cursor };
static const int refreshrate = 60;
static void (*handler[LASTEvent])(XEvent *);
static struct {
	XEvent queue[QUEUE_SIZE];
	size_t queued, consumed;
	int grabbed, grabs, ungrabs, grab_result, pointer_ok, queries;
	int startx, starty, syncs, flushes, publishes, enter_checks;
	int coalesced, notifies;
	int resizes, arrangements, moves, raises, updates, dispatches;
} f;
static void dwindle(Monitor *);
static void tile(Monitor *m) { (void)m; }
static void monocle(Monitor *m) { (void)m; }
static const Layout bsp_layout = { dwindle }, tile_layout = { tile }, floating_layout = { NULL };
static const Layout monocle_layout = { monocle };
static Client *wintoclient(Window);
static void configure(Client *c);
static void arrange(Monitor *);
static int getrootptr(int *, int *);
static int qs_dock_event(XEvent *);
static void qs_publish(void);
static void focus(Client *);
static Monitor *recttomon(int, int, int, int);
static void updateclientlist(void);
#endif
