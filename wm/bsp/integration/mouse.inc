/* No Client, Monitor, workspace or node pointer survives event dispatch. */
static BspWorkspace *
bsp_resize_source(Window window, int monitor, unsigned int tagsmask,
                  const BspResize *snapshot)
{
	Client *c = wintoclient(window);
	BspWorkspace *view;
	BspRect b;

	if (!c || !c->mon || c->mon->num != monitor
	|| c->mon->tagset[c->mon->seltags] != tagsmask
	|| !ISVISIBLE(c) || c->isfloating || c->isfullscreen
	|| c->mon->lt[c->mon->sellt]->arrange != dwindle)
		return NULL;
	view = bsp_active(c->mon);
	if (!view || view->tree.generation != snapshot->generation)
		return NULL;
	b = view->tree.bounds;
	if (b.x != snapshot->bounds.x || b.y != snapshot->bounds.y
	|| b.width != snapshot->bounds.width || b.height != snapshot->bounds.height)
		return NULL;
	return view;
}

static void
bsp_resize_mouse(Client *initial)
{
	const long events = MOUSEMASK | ExposureMask | SubstructureRedirectMask
	                  | SubstructureNotifyMask | StructureNotifyMask | PropertyChangeMask;
	Window source = initial->win;
	int monitor = initial->mon->num;
	unsigned int tagsmask = initial->mon->tagset[initial->mon->seltags];
	BspWorkspace *view;
	BspResize snapshot;
	Client *c;
	XEvent ev;
	Time lasttime = 0;
	int x, y;

	if (initial->isfloating || initial->isfullscreen || !ISVISIBLE(initial)
	|| initial->mon->lt[initial->mon->sellt]->arrange != dwindle
	|| !(view = bsp_active(initial->mon))
	|| !bsp_resize_begin(&view->tree, source, 0, 0, &snapshot))
		return;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
	                None, cursor[CurResize]->cursor, CurrentTime) != GrabSuccess)
		return;
	if (!getrootptr(&x, &y))
		goto ungrab;
	view = bsp_resize_source(source, monitor, tagsmask, &snapshot);
	if (!view || !bsp_resize_begin(&view->tree, source, x, y, &snapshot))
		goto ungrab;
	for (;;) {
		XMaskEvent(dpy, events, &ev);
		if (ev.type == MotionNotify || ev.type == ButtonRelease) {
			if (ev.type == ButtonRelease && ev.xbutton.button != Button3)
				continue;
			view = bsp_resize_source(source, monitor, tagsmask, &snapshot);
			if (!view)
				break;
			if (ev.type == MotionNotify) {
				if (ev.xmotion.time - lasttime <= (1000 / refreshrate))
					continue;
				lasttime = ev.xmotion.time;
				x = ev.xmotion.x_root; y = ev.xmotion.y_root;
			} else {
				x = ev.xbutton.x_root; y = ev.xbutton.y_root;
			}
			if (!bsp_resize_update(&view->tree, &snapshot, x, y))
				break;
			if ((c = wintoclient(source)))
				arrange(c->mon);
			if (ev.type == ButtonRelease)
				break;
		} else if (ev.type != ButtonPress) {
			if (!qs_dock_event(&ev) && ev.type >= 0 && ev.type < LASTEvent && handler[ev.type])
				handler[ev.type](&ev);
			if (!bsp_resize_source(source, monitor, tagsmask, &snapshot))
				break;
		}
		qs_publish();
		XFlush(dpy);
	}
ungrab:
	XUngrabPointer(dpy, CurrentTime);
	XSync(dpy, False);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
	qs_publish();
}
