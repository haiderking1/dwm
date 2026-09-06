#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef unsigned long Window;
typedef struct Monitor Monitor;
typedef struct Client Client;
typedef struct { int unused; } Layout;
struct Client {
	Window win;
	int x, y, w, h, oldx, oldy, oldw, oldh, bw, oldbw;
	int isfloating, isfullscreen, oldstate;
	unsigned int tags;
	Monitor *mon;
	Client *next, *snext;
};
struct Monitor {
	int num, nmaster, showbar, topbar, mx, my, mw, mh, wx, by, ww;
	unsigned int tagset[2], seltags, sellt;
	float mfact;
	Window barwin;
	const Layout *lt[2];
	Client *clients, *stack, *sel;
	Monitor *next;
};
#define LENGTH(a) (sizeof(a) / sizeof((a)[0]))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define TAGMASK 1023U
#define None 0
static Layout layouts[3];
static Monitor *mons, *selmon;
static int borderpx = 1, bh = 20, dpy;
static Client *wintoclient(Window win) {
	Monitor *m; Client *c;
	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next) if (c->win == win) return c;
	return NULL;
}
static void attach(Client *c) { c->next = c->mon->clients; c->mon->clients = c; }
static void attachstack(Client *c) { c->snext = c->mon->stack; c->mon->stack = c; }
static void detach(Client *c) {
	Client **p;
	for (p = &c->mon->clients; *p && *p != c; p = &(*p)->next);
	assert(*p == c); *p = c->next;
}
static void detachstack(Client *c) {
	Client **p;
	for (p = &c->mon->stack; *p && *p != c; p = &(*p)->snext);
	assert(*p == c); *p = c->snext;
	if (c->mon->sel == c) c->mon->sel = c->mon->stack;
}
static void resizeclient(Client *c, int x, int y, int w, int h) {
	c->oldx=c->x; c->oldy=c->y; c->oldw=c->w; c->oldh=c->h;
	c->x=x; c->y=y; c->w=w; c->h=h;
}
static void updatebarpos(Monitor *m) { (void)m; }
static void XMoveResizeWindow(int display, Window win, int x, int y, int w, int h) {
	(void)display; (void)win; (void)x; (void)y; (void)w; (void)h;
}
static void focus(Client *c) { selmon->sel = c; }
static void arrange(Monitor *m) { (void)m; }
#include "../../reload/state.h"
#include "../../reload/save.inc"
#include "../../reload/restore.inc"
