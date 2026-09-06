static void
resize_tiled(Client *initial)
{
	const long events = MOUSEMASK | ExposureMask | SubstructureRedirectMask
	                  | SubstructureNotifyMask | StructureNotifyMask | PropertyChangeMask;
	Window source = initial->win;
	TileResize state;
	Client *c;
	XEvent ev;
	Time lasttime = 0;
	int x, y, dock;

	/* Unsupported layouts and a lone tile must not grab or warp the cursor. */
	if (!resize_eligible(initial) || resize_count(initial->mon) < 2)
		return;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
	                None, cursor[CurResize]->cursor, CurrentTime) != GrabSuccess)
		return;
	if (!getrootptr(&x, &y)) {
		XUngrabPointer(dpy, CurrentTime);
		return;
	}
	c = wintoclient(source);
	if (!c || !resize_begin(&state, c, x, y)) {
		XUngrabPointer(dpy, CurrentTime);
		return;
	}
	for (;;) {
		XMaskEvent(dpy, events, &ev);
		if (ev.type == MotionNotify || ev.type == ButtonRelease) {
			if (ev.type == ButtonRelease && ev.xbutton.button != Button3)
				continue;
			mouse_coalesce_motion(&ev);
			if (ev.type == MotionNotify) {
				if (ev.xmotion.time - lasttime <= (1000 / refreshrate))
					continue;
				lasttime = ev.xmotion.time;
				x = ev.xmotion.x_root; y = ev.xmotion.y_root;
			} else {
				/* Release coordinates are authoritative even when the final
				 * motion was throttled, or there was no MotionNotify at all. */
				x = ev.xbutton.x_root; y = ev.xbutton.y_root;
			}
			if (!resize_update(&state, x, y))
				break;
			if (ev.type == ButtonRelease)
				break;
		} else if (ev.type != ButtonPress) {
			dock = qs_dock_event(&ev);
			if (!dock && ev.type >= 0 && ev.type < LASTEvent && handler[ev.type])
				handler[ev.type](&ev);
			/* This also catches a freed neighbor, insertion, reorder, tag,
			 * layout, fullscreen, monitor removal or workarea change. */
			if (!resize_topology(&state))
				break;
			if (mouse_publish_event(&ev, dock))
				qs_publish();
			XFlush(dpy);
		}
	}
	free(state.slots);
	XUngrabPointer(dpy, CurrentTime);
	XSync(dpy, False);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
	qs_publish();
}
