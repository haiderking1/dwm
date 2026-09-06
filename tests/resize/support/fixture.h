#ifndef RESIZE_TEST_FIXTURE_H
#define RESIZE_TEST_FIXTURE_H
#include <X11/Xlib.h>
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Client Client;
typedef struct Monitor Monitor;
typedef struct { void (*arrange)(Monitor *); } Layout;
struct Client {
	int oldx, oldy, oldw, oldh;
	Window win;
	float cfact;
	int x, y, w, h, bw, isfloating, isfullscreen;
	int incw, inch, minw, minh;
	unsigned int tags;
	Client *next, *snext;
	Monitor *mon;
};
struct Monitor {
	Client *clients, *stack, *sel;
	int wx, wy, ww, wh, nmaster;
	float mfact;
	unsigned int tagset[2], seltags, sellt;
	const Layout *lt[2];
};
#define MAX(A,B) ((A) > (B) ? (A) : (B))
#define MIN(A,B) ((A) < (B) ? (A) : (B))
#define ISVISIBLE(C) ((C)->tags & (C)->mon->tagset[(C)->mon->seltags])
#define WIDTH(C) ((C)->w + 2 * (C)->bw)
#define HEIGHT(C) ((C)->h + 2 * (C)->bw)
#define MOUSEMASK (ButtonPressMask | ButtonReleaseMask | PointerMotionMask)
#define CAPACITY 16
#define QUEUE_SIZE 128

typedef struct {
	Monitor *mon;
	Client *clients[CAPACITY];
	XEvent queue[QUEUE_SIZE];
	size_t queued, consumed;
	int grab_result, pointer_ok, grabbed, grabs, ungrabs, queries;
	int startx, starty, syncs, flushes, publishes, enter_checks;
	int coalesced, notifies;
	int resizes, arrangements, dispatches, docks;
} Fixture;
static Fixture f;
static Display *dpy;
static const Window root = 1;
enum { CurResize };
typedef struct { Cursor cursor; } TestCursor;
static TestCursor resize_cursor = { 77 };
static TestCursor *cursor[] = { &resize_cursor };
static const int refreshrate = 60;
static void (*handler[LASTEvent])(XEvent *);
static void tile(Monitor *m);
static void monocle(Monitor *m);
static const Layout tiled_layout = { tile };
static const Layout monocle_layout = { monocle };
static const Layout floating_layout = { NULL };
static Client *nexttiled(Client *c);
static Client *wintoclient(Window w);
static void configure(Client *c);
static void arrange(Monitor *m);
static int getrootptr(int *x, int *y);
static int qs_dock_event(XEvent *ev);
static void qs_publish(void);
static void dispatch_event(XEvent *ev);
#endif
