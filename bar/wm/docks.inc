/* Included after config.h; dwm keeps ownership of Monitor and Client. */
#include "geometry.h"

typedef struct QsDock QsDock;
struct QsDock {
	Window win;
	int mapped;
	uint32_t strut[12];
	QsDock *next;
};
static QsDock *qs_docks;

static QsDock *
qs_dock_find(Window win)
{
	QsDock *dock;
	for (dock = qs_docks; dock && dock->win != win; dock = dock->next);
	return dock;
}

static int
qs_property(Window win, Atom property, Atom type, unsigned long *values, int count)
{
	Atom actual;
	int format, ok = 0;
	unsigned long n, extra;
	unsigned char *data = NULL;
	if (XGetWindowProperty(dpy, win, property, 0, count, False, type,
	                      &actual, &format, &n, &extra, &data) == Success &&
	    actual == type && format == 32 && n == (unsigned long)count && !extra) {
		memcpy(values, data, count * sizeof *values);
		ok = 1;
	}
	if (data) XFree(data);
	return ok;
}

static int
qs_is_dock(Window win)
{
	Atom actual;
	int format, found = 0;
	unsigned long n, extra, i;
	unsigned char *data = NULL;
	if (XGetWindowProperty(dpy, win, netatom[NetWMWindowType], 0, 1024,
	                      False, XA_ATOM, &actual, &format, &n, &extra, &data) == Success &&
	    actual == XA_ATOM && format == 32)
		for (i = 0; i < n; i++)
			if (((Atom *)data)[i] == netatom[NetWMWindowTypeDock]) found = 1;
	if (data) XFree(data);
	return found;
}

static void
qs_dock_read(QsDock *dock)
{
	unsigned long values[12] = {0};
	int i;
	if (!qs_property(dock->win, netatom[NetWMStrutPartial], XA_CARDINAL, values, 12)) {
		memset(values, 0, sizeof values);
		qs_property(dock->win, netatom[NetWMStrut], XA_CARDINAL, values, 4);
		/* Legacy struts span the entire root, including after a resize. */
		values[5] = values[7] = UINT32_MAX;
		values[9] = values[11] = UINT32_MAX;
	}
	for (i = 0; i < 12; i++) dock->strut[i] = (uint32_t)values[i];
}

static void
qs_dock_area(Monitor *m)
{
	QsDock *dock;
	QsRect bounds = {m->mx, m->my, m->mw, m->mh}, area = bounds;
	for (dock = qs_docks; dock; dock = dock->next)
		if (dock->mapped) qs_strut_apply(bounds, sw, sh, dock->strut, &area);
	m->wx = area.x; m->wy = area.y;
	m->ww = area.width; m->wh = area.height;
}

static void
qs_dock_reflow(void)
{
	Monitor *m;
	for (m = mons; m; m = m->next) {
		updatebarpos(m);
		if (!externalbar && m->barwin != None)
			XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, m->ww, bh);
	}
	arrange(NULL);
}

static void
qs_dock_remove(Window win)
{
	QsDock **link, *dock;
	for (link = &qs_docks; (dock = *link); link = &dock->next)
		if (dock->win == win) {
			*link = dock->next;
			free(dock);
			qs_dock_reflow();
			return;
		}
}

/* Watch all root children so a dock type set after creation is not missed. */
static int
qs_dock_watch(Window win)
{
	XWindowAttributes wa;
	QsDock *dock;
	Monitor *m;
	Client *c;
	if (win == root || win == wmcheckwin) return 0;
	for (m = mons; m; m = m->next) if (win == m->barwin) return 0;
	if (!XGetWindowAttributes(dpy, win, &wa)) return 0;
	XSelectInput(dpy, win, wa.your_event_mask | PropertyChangeMask);
	if (!qs_is_dock(win)) return 0;
	if ((c = wintoclient(win))) {
		unmanage(c, 0);
		XSelectInput(dpy, win, PropertyChangeMask);
	}
	if (!(dock = qs_dock_find(win))) {
		dock = ecalloc(1, sizeof *dock);
		dock->win = win;
		dock->next = qs_docks;
		qs_docks = dock;
	}
	dock->mapped = wa.map_state == IsViewable;
	qs_dock_read(dock);
	qs_dock_reflow();
	return 1;
}

static int
qs_dock_event(XEvent *event)
{
	Window win;
	QsDock *dock;
	XWindowAttributes wa;
	switch (event->type) {
	case CreateNotify:
		if (event->xcreatewindow.parent == root) qs_dock_watch(event->xcreatewindow.window);
		break;
	case MapNotify:
		qs_dock_watch(event->xmap.window);
		break;
	case UnmapNotify:
		if ((dock = qs_dock_find(event->xunmap.window))) {
			dock->mapped = 0;
			qs_dock_reflow();
			return 1;
		}
		break;
	case DestroyNotify:
		if (qs_dock_find(event->xdestroywindow.window)) {
			qs_dock_remove(event->xdestroywindow.window);
			return 1;
		}
		break;
	case ReparentNotify:
		if (event->xreparent.parent != root) qs_dock_remove(event->xreparent.window);
		else qs_dock_watch(event->xreparent.window);
		break;
	case PropertyNotify:
		win = event->xproperty.window;
		if (event->xproperty.atom == netatom[NetWMWindowType]) {
			if (qs_dock_watch(win)) return 1;
			if (qs_dock_find(win)) {
				qs_dock_remove(win);
				if (XGetWindowAttributes(dpy, win, &wa) && !wa.override_redirect &&
				    wa.map_state == IsViewable && !wintoclient(win)) manage(win, &wa);
			}
		} else if ((event->xproperty.atom == netatom[NetWMStrutPartial] ||
		            event->xproperty.atom == netatom[NetWMStrut]) &&
		           (dock = qs_dock_find(win))) {
			qs_dock_read(dock);
			qs_dock_reflow();
			return 1;
		}
		break;
	}
	return 0;
}

static void
qs_dock_cleanup(void)
{
	QsDock *dock;
	while ((dock = qs_docks)) { qs_docks = dock->next; free(dock); }
}
