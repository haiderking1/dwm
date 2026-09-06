/* Tiled dragging changes only the X window's position until release. The
 * logical geometry and list slot remain intact, so an empty drop is reversible. */
static void
drag_tiled(Client *initial)
{
	enum { DragThreshold = 4 };
	const long events = MOUSEMASK | ExposureMask | SubstructureRedirectMask
	                  | SubstructureNotifyMask | StructureNotifyMask | PropertyChangeMask;
	Window window = initial->win;
	Client *c, *target;
	Monitor *origin, *destination;
	XEvent ev;
	Time lasttime = 0;
	int startx, starty, px, py, ox = initial->x, oy = initial->y;
	int moved = 0, released = 0;

	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
	                None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess)
		return;
	if (!getrootptr(&startx, &starty)) {
		XUngrabPointer(dpy, CurrentTime);
		return;
	}
	px = startx;
	py = starty;
	for (;;) {
		XMaskEvent(dpy, events, &ev);
		if (ev.type == MotionNotify || ev.type == ButtonRelease) {
			if (ev.type == ButtonRelease && ev.xbutton.button != Button1)
				continue;
			px = ev.type == MotionNotify ? ev.xmotion.x_root : ev.xbutton.x_root;
			py = ev.type == MotionNotify ? ev.xmotion.y_root : ev.xbutton.y_root;
			if (abs(px - startx) >= DragThreshold || abs(py - starty) >= DragThreshold)
				moved = 1;
			if (ev.type == ButtonRelease) {
				released = 1;
				break;
			}
			if (ev.xmotion.time - lasttime <= (1000 / refreshrate))
				continue;
			lasttime = ev.xmotion.time;
		} else if (ev.type != ButtonPress) {
			/* Handle closure, mapping, dock struts and monitor changes while
			 * grabbed. Never retain a Client pointer across event dispatch. */
			if (!qs_dock_event(&ev) && ev.type >= 0 && ev.type < LASTEvent && handler[ev.type])
				handler[ev.type](&ev);
		}
		c = wintoclient(window);
		if (!drag_eligible(c))
			break;
		if (moved) {
			XMoveWindow(dpy, window, ox + px - startx, oy + py - starty);
			XRaiseWindow(dpy, window);
		}
		qs_publish();
		XFlush(dpy);
	}
	XUngrabPointer(dpy, CurrentTime);
	if ((c = wintoclient(window))) {
		origin = c->mon;
		/* Force restoration: arrange can skip resize when its logical
		 * rectangle is unchanged, but the preview moved the actual window. */
		XMoveWindow(dpy, window, c->x, c->y);
		if (released && moved && drag_eligible(c)) {
			target = drag_target(recttomon(px, py, 1, 1), c, px, py);
			destination = target ? target->mon : NULL;
			if (bsp_drag_prepare(c, target) && drag_swap(c, target)) {
				bsp_drag_swap(c, target, origin, destination);
				updateclientlist();
			}
		}
		focus(c);
		arrange(origin);
		if (c->mon != origin)
			arrange(c->mon);
	}
	/* Do not let EnterNotify events from the preview steal final focus. */
	XSync(dpy, False);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
	qs_publish();
}
