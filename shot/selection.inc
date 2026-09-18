/*
 * Region selection. Print freezes the screen into a pixmap and maps one
 * fullscreen canvas showing it. The rubber band is then four filled strips
 * drawn on that canvas, erased by copying the freeze back. Other clients
 * are not restacked or exposed during the drag, which is what made the
 * previous overlay windows stutter over a browser or a game. The PNG is
 * cropped from the freeze, so it matches what the user selected.
 */

#define SHOT_SELECT_MASK (MOUSEMASK | KeyPressMask | ExposureMask \
	| SubstructureRedirectMask | SubstructureNotifyMask \
	| StructureNotifyMask | PropertyChangeMask)
#define SHOT_BAND_THICKNESS 2

enum { SHOT_CANCEL, SHOT_REGION, SHOT_MON, SHOT_GRABFAIL };

static Window shot_canvas;
static Pixmap shot_bg;
static GC shot_copy_gc, shot_fill_gc;
static int shot_canvas_on;
static unsigned int shot_canvas_w, shot_canvas_h;
static Cur *shot_cross;
static int shot_band_drawn;
static int shot_busy;
static struct shot_rect shot_band_prev[SHOT_BAND_EDGES];
static int shot_last_px, shot_last_py;
static struct { int x, y, w, h; } shot_band;

static Cur *
shot_crosshair(void)
{
	if (!shot_cross)
		shot_cross = drw_cur_create(drw, XC_crosshair);
	return shot_cross;
}

static GC
shot_gc_copy(void)
{
	XGCValues gcv;

	if (!shot_copy_gc) {
		gcv.graphics_exposures = False;
		shot_copy_gc = XCreateGC(dpy, root, GCGraphicsExposures, &gcv);
	}
	return shot_copy_gc;
}

static GC
shot_gc_fill(void)
{
	XGCValues gcv;

	if (!shot_fill_gc) {
		gcv.foreground = scheme[SchemeSel][ColBorder].pixel;
		gcv.graphics_exposures = False;
		shot_fill_gc = XCreateGC(dpy, root, GCForeground|GCGraphicsExposures, &gcv);
	}
	return shot_fill_gc;
}

static void
shot_freeze_blit_client(Client *c)
{
	XWindowAttributes wa;
	XImage *img;
	int x, y, w, h, sx = 0, sy = 0;

	if (!shot_bg || !c)
		return;
	if (!XGetWindowAttributes(dpy, c->win, &wa) || wa.map_state != IsViewable
			|| wa.width < 1 || wa.height < 1)
		return;
	x = wa.x + wa.border_width;
	y = wa.y + wa.border_width;
	w = wa.width;
	h = wa.height;
	if (x < 0) { w += x; sx = -x; x = 0; }
	if (y < 0) { h += y; sy = -y; y = 0; }
	if (x + w > sw)
		w = sw - x;
	if (y + h > sh)
		h = sh - y;
	if (w < 1 || h < 1)
		return;
	XSetErrorHandler(xerrordummy);
	img = XGetImage(dpy, c->win, sx, sy, (unsigned int)w, (unsigned int)h,
		AllPlanes, ZPixmap);
	XSync(dpy, False);
	XSetErrorHandler(xerror);
	if (!img)
		return;
	XPutImage(dpy, shot_bg, shot_gc_copy(), img, 0, 0, x, y,
		(unsigned int)w, (unsigned int)h);
	XDestroyImage(img);
}

static int
shot_freeze(void)
{
	XGCValues gcv;
	XSetWindowAttributes wa;
	Monitor *m;
	GC snap;
	unsigned int mask;
	int depth;

	if (sw < 1 || sh < 1)
		return 0;
	depth = DefaultDepth(dpy, screen);
	if (shot_canvas)
		XSetWindowBackgroundPixmap(dpy, shot_canvas, None);
	if (shot_bg) {
		XFreePixmap(dpy, shot_bg);
		shot_bg = None;
	}
	XSetErrorHandler(xerrordummy);
	shot_bg = XCreatePixmap(dpy, root, (unsigned int)sw, (unsigned int)sh, depth);
	XSync(dpy, False);
	XSetErrorHandler(xerror);
	if (!shot_bg)
		return 0;
	gcv.subwindow_mode = IncludeInferiors;
	gcv.graphics_exposures = False;
	snap = XCreateGC(dpy, root, GCSubwindowMode|GCGraphicsExposures, &gcv);
	XSetErrorHandler(xerrordummy);
	XCopyArea(dpy, root, shot_bg, snap, 0, 0, (unsigned int)sw,
		(unsigned int)sh, 0, 0);
	XSync(dpy, False);
	XSetErrorHandler(xerror);
	XFreeGC(dpy, snap);
	for (m = mons; m; m = m->next)
		if (m->sel && m->sel->isfullscreen)
			shot_freeze_blit_client(m->sel);
	wa.override_redirect = True;
	wa.background_pixmap = shot_bg;
	wa.event_mask = ExposureMask;
	wa.cursor = shot_crosshair()->cursor;
	mask = CWOverrideRedirect|CWBackPixmap|CWEventMask|CWCursor;
	if (!shot_canvas) {
		shot_canvas = XCreateWindow(dpy, root, 0, 0, (unsigned int)sw,
			(unsigned int)sh, 0, depth, InputOutput, CopyFromParent,
			mask, &wa);
	} else if (shot_canvas_w != (unsigned int)sw
			|| shot_canvas_h != (unsigned int)sh) {
		XMoveResizeWindow(dpy, shot_canvas, 0, 0, (unsigned int)sw,
			(unsigned int)sh);
		XChangeWindowAttributes(dpy, shot_canvas, mask, &wa);
	} else {
		XSetWindowBackgroundPixmap(dpy, shot_canvas, shot_bg);
	}
	shot_canvas_w = (unsigned int)sw;
	shot_canvas_h = (unsigned int)sh;
	XMapRaised(dpy, shot_canvas);
	XClearWindow(dpy, shot_canvas);
	shot_canvas_on = 1;
	shot_band_drawn = 0;
	return 1;
}

static void
shot_canvas_hide(void)
{
	if (shot_canvas && shot_canvas_on) {
		XUnmapWindow(dpy, shot_canvas);
		shot_canvas_on = 0;
	}
}

static void
shot_freeze_destroy(void)
{
	shot_canvas_hide();
	if (shot_canvas)
		XSetWindowBackgroundPixmap(dpy, shot_canvas, None);
	if (shot_bg) {
		XFreePixmap(dpy, shot_bg);
		shot_bg = None;
	}
	shot_band_drawn = 0;
}

static void
shot_band_restore(void)
{
	GC gc;
	int i;

	if (!shot_bg || !shot_canvas || !shot_band_drawn)
		return;
	gc = shot_gc_copy();
	for (i = 0; i < SHOT_BAND_EDGES; i++)
		if (shot_band_prev[i].w > 0 && shot_band_prev[i].h > 0)
			XCopyArea(dpy, shot_bg, shot_canvas, gc,
				shot_band_prev[i].x, shot_band_prev[i].y,
				(unsigned int)shot_band_prev[i].w,
				(unsigned int)shot_band_prev[i].h,
				shot_band_prev[i].x, shot_band_prev[i].y);
}

static void
shot_band_paint(const struct shot_rect *e)
{
	GC gc;
	int i;

	if (!shot_canvas)
		return;
	gc = shot_gc_fill();
	for (i = 0; i < SHOT_BAND_EDGES; i++)
		if (e[i].w > 0 && e[i].h > 0)
			XFillRectangle(dpy, shot_canvas, gc, e[i].x, e[i].y,
				(unsigned int)e[i].w, (unsigned int)e[i].h);
}

static void
shot_band_apply(int show)
{
	struct shot_rect e[SHOT_BAND_EDGES];
	int i;

	if (!shot_canvas || !shot_bg)
		return;
	shot_band_restore();
	shot_band_drawn = 0;
	if (!show || shot_band.w < 1 || shot_band.h < 1)
		return;
	shot_band_edges(shot_band.x, shot_band.y, shot_band.w, shot_band.h,
		SHOT_BAND_THICKNESS, e);
	shot_band_paint(e);
	for (i = 0; i < SHOT_BAND_EDGES; i++)
		shot_band_prev[i] = e[i];
	shot_band_drawn = 1;
}

static void
shot_canvas_expose(const XEvent *ev)
{
	if (!shot_canvas || !shot_bg || ev->xexpose.window != shot_canvas)
		return;
	if (ev->xexpose.count)
		return;
	XCopyArea(dpy, shot_bg, shot_canvas, shot_gc_copy(), 0, 0,
		shot_canvas_w, shot_canvas_h, 0, 0);
	if (shot_band_drawn)
		shot_band_paint(shot_band_prev);
}

static void
shot_select_reset_draw(void)
{
	int i;

	shot_last_px = INT_MIN;
	shot_last_py = INT_MIN;
	shot_band_drawn = 0;
	for (i = 0; i < SHOT_BAND_EDGES; i++) {
		shot_band_prev[i].x = 0;
		shot_band_prev[i].y = 0;
		shot_band_prev[i].w = 0;
		shot_band_prev[i].h = 0;
	}
}

static void
shot_band_from_drag(int x0, int y0, int x, int y)
{
	shot_band.x = x0 < x ? x0 : x;
	shot_band.y = y0 < y ? y0 : y;
	shot_band.w = (x0 < x ? x - x0 : x0 - x) + 1;
	shot_band.h = (y0 < y ? y - y0 : y0 - y) + 1;
}

static int
shot_drag_is_region(int x0, int y0, int x, int y)
{
	return (x - x0 >= 2 || x0 - x >= 2)
		&& (y - y0 >= 2 || y0 - y >= 2);
}

static void
shot_drag_frame(int x0, int y0, int x, int y)
{
	if (x == shot_last_px && y == shot_last_py)
		return;
	shot_band_from_drag(x0, y0, x, y);
	shot_band_apply(1);
	shot_last_px = x;
	shot_last_py = y;
	XFlush(dpy);
}

static void
shot_cookie_done(XEvent *ev)
{
	if (ev->type == GenericEvent && ev->xcookie.data)
		XFreeEventData(dpy, &ev->xcookie);
}

static int
shot_cookie(XEvent *ev)
{
	if (ev->type != GenericEvent)
		return 0;
	if (ev->xcookie.data)
		return 1;
	return XGetEventData(dpy, &ev->xcookie) ? 1 : 0;
}

static void
shot_coalesce_raw_motion(XEvent *ev)
{
	XEvent next;
	unsigned int n = 256;

	while (n-- && XEventsQueued(dpy, QueuedAfterReading) > 0) {
		XPeekEvent(dpy, &next);
		if (next.type != GenericEvent
				|| next.xcookie.extension != input_xi_opcode()
				|| next.xcookie.evtype != XI_RawMotion)
			break;
		shot_cookie_done(ev);
		XNextEvent(dpy, ev);
		if (!shot_cookie(ev))
			break;
	}
}

/*
 * In-game selection. Another client owns the pointer grab, so there is no
 * grab here: raw pointer events are the only input that survives a grab.
 * The freeze canvas is already mapped; the game can page-flip underneath
 * it without touching the pixels the user is selecting.
 */
static int
shot_select_raw(int *rx, int *ry, int *rw, int *rh)
{
	XEvent ev;
	int x0 = 0, y0 = 0, x = 0, y = 0;
	int selecting = 0, result = SHOT_CANCEL;

	if (!input_raw_pointer_select(1))
		return SHOT_GRABFAIL;
	for (;;) {
		XIRawEvent *raw;
		int done = 0;

		if (!XCheckTypedEvent(dpy, GenericEvent, &ev))
			XNextEvent(dpy, &ev);
		if (ev.type != GenericEvent) {
			int dock;

			if (ev.type == Expose) {
				shot_canvas_expose(&ev);
				continue;
			}
			dock = qs_dock_event(&ev);
			if (!dock && ev.type >= 0 && ev.type < LASTEvent && handler[ev.type])
				handler[ev.type](&ev);
			continue;
		}
		if (ev.xcookie.extension != input_xi_opcode())
			continue;
		switch (ev.xcookie.evtype) {
		case XI_RawButtonPress:
		case XI_RawButtonRelease:
		case XI_RawMotion:
		case XI_RawKeyPress:
			break;
		default:
			continue;
		}
		if (!shot_cookie(&ev))
			continue;
		if (ev.xcookie.evtype == XI_RawMotion)
			shot_coalesce_raw_motion(&ev);
		raw = ev.xcookie.data;
		if (!raw) {
			shot_cookie_done(&ev);
			continue;
		}
		if (ev.xcookie.evtype == XI_RawKeyPress) {
			if (XKeycodeToKeysym(dpy, (KeyCode)raw->detail, 0) == XK_Escape)
				done = 1;
		} else if (ev.xcookie.evtype == XI_RawButtonPress) {
			if (raw->detail == Button1 && !selecting) {
				selecting = 1;
				if (!getrootptr(&x0, &y0)) {
					selecting = 0;
					done = 1;
				}
				x = x0;
				y = y0;
			} else if (raw->detail == Button2 || raw->detail == Button3) {
				done = 1;
			}
		} else if (ev.xcookie.evtype == XI_RawButtonRelease) {
			if (raw->detail == Button1 && selecting) {
				if (!getrootptr(&x, &y)) {
					x = x0;
					y = y0;
				}
				shot_band_from_drag(x0, y0, x, y);
				if (shot_drag_is_region(x0, y0, x, y))
					result = SHOT_REGION;
				else
					result = SHOT_MON;
				*rx = shot_band.x;
				*ry = shot_band.y;
				*rw = shot_band.w;
				*rh = shot_band.h;
				done = 1;
			}
		} else if (selecting) {
			if (getrootptr(&x, &y))
				shot_drag_frame(x0, y0, x, y);
		}
		shot_cookie_done(&ev);
		if (done)
			break;
	}
	shot_band_apply(0);
	input_raw_pointer_select(0);
	XSync(dpy, False);
	return result;
}

static int
shot_select(int *rx, int *ry, int *rw, int *rh)
{
	XEvent ev;
	int x0 = 0, y0 = 0, x = 0, y = 0;
	int selecting = 0, grabbed_kb = 0, result = SHOT_CANCEL;
	Window grabwin;

	if (shot_busy)
		return SHOT_CANCEL;
	shot_busy = 1;
	shot_select_reset_draw();
	shot_freeze();
	grabwin = shot_canvas_on ? shot_canvas : root;
	if (XGrabPointer(dpy, grabwin, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
			None, shot_crosshair()->cursor, CurrentTime) != GrabSuccess) {
		result = shot_select_raw(rx, ry, rw, rh);
		shot_canvas_hide();
		shot_busy = 0;
		return result;
	}
	if (XGrabKeyboard(dpy, grabwin, False, GrabModeAsync, GrabModeAsync,
			CurrentTime) == GrabSuccess)
		grabbed_kb = 1;
	if (!getrootptr(&x, &y))
		goto out;
	XSync(dpy, False);
	for (;;) {
		if (!XCheckMaskEvent(dpy, MOUSEMASK | KeyPressMask, &ev))
			XMaskEvent(dpy, SHOT_SELECT_MASK, &ev);
		if (ev.type == KeyPress) {
			if (XKeycodeToKeysym(dpy, (KeyCode)ev.xkey.keycode, 0) == XK_Escape) {
				result = SHOT_CANCEL;
				break;
			}
			continue;
		}
		if (ev.type == ButtonPress) {
			if (ev.xbutton.button == Button1 && !selecting) {
				selecting = 1;
				x0 = x = ev.xbutton.x_root;
				y0 = y = ev.xbutton.y_root;
			} else if (ev.xbutton.button == Button2
					|| ev.xbutton.button == Button3) {
				result = SHOT_CANCEL;
				break;
			}
			continue;
		}
		if (ev.type == ButtonRelease && ev.xbutton.button == Button1) {
			if (selecting) {
				x = ev.xbutton.x_root;
				y = ev.xbutton.y_root;
				shot_band_from_drag(x0, y0, x, y);
				if (shot_drag_is_region(x0, y0, x, y))
					result = SHOT_REGION;
				else
					result = SHOT_MON;
				*rx = shot_band.x;
				*ry = shot_band.y;
				*rw = shot_band.w;
				*rh = shot_band.h;
			} else {
				result = SHOT_MON;
			}
			break;
		}
		if (ev.type == MotionNotify) {
			mouse_coalesce_motion(&ev);
			x = ev.xmotion.x_root;
			y = ev.xmotion.y_root;
			if (selecting)
				shot_drag_frame(x0, y0, x, y);
			continue;
		}
		if (ev.type == Expose) {
			shot_canvas_expose(&ev);
			continue;
		}
		{
			int dock = qs_dock_event(&ev);

			if (!dock && ev.type >= 0 && ev.type < LASTEvent && handler[ev.type])
				handler[ev.type](&ev);
		}
	}
out:
	shot_band_apply(0);
	shot_canvas_hide();
	XUngrabPointer(dpy, CurrentTime);
	if (grabbed_kb)
		XUngrabKeyboard(dpy, CurrentTime);
	XSync(dpy, False);
	shot_busy = 0;
	return result;
}
