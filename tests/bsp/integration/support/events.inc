enum { ChangeFloat = 1, ChangeFullscreen, ChangeLayout, ChangeView, ChangeBounds,
       ChangeRotate, ChangeNeighbor, ChangeFocus, ChangeMonitor };
static XEvent *
push_event(int type)
{
	XEvent *ev;
	assert(f.queued < QUEUE_SIZE);
	ev = &f.queue[f.queued++];
	memset(ev, 0, sizeof(*ev)); ev->type = type;
	return ev;
}

static void
push_pointer(int type, unsigned int button, int x, int y, Time time)
{
	XEvent *ev = push_event(type);
	if (type == MotionNotify) {
		ev->xmotion.x_root = x; ev->xmotion.y_root = y;
		ev->xmotion.x = ev->xmotion.y = -9999; ev->xmotion.time = time;
	} else {
		ev->xbutton.x_root = x; ev->xbutton.y_root = y;
		ev->xbutton.x = ev->xbutton.y = -9999;
		ev->xbutton.button = button; ev->xbutton.time = time;
	}
}

static void
dispatch_event(XEvent *ev)
{
	Client *c = wintoclient(1);
	++f.dispatches;
	switch (ev->type) {
	case DestroyNotify: remove_client(ev->xdestroywindow.window); break;
	case UnmapNotify: remove_client(ev->xunmap.window); break;
	case MapRequest: add_client(selmon, 99, 1); break;
	case PropertyNotify:
		switch (ev->xproperty.atom) {
		case ChangeFloat: c->isfloating = 1; break;
		case ChangeFullscreen: c->isfullscreen = c->isfloating = 1; break;
		case ChangeLayout: selmon->lt[0] = &tile_layout; break;
		case ChangeView: selmon->tagset[0] = 3; break;
		case ChangeBounds: selmon->ww -= 100; break;
		case ChangeRotate: bsp_rotate_selected(NULL); break;
		case ChangeNeighbor: remove_client(2); break;
		case ChangeFocus: focus(wintoclient(2)); break;
		case ChangeMonitor: selmon->num = 7; break;
		default: assert(0);
		}
		break;
	case Expose: break;
	default: assert(0);
	}
}

static int
qs_dock_event(XEvent *ev)
{
	if (ev->type != ConfigureNotify)
		return 0;
	selmon->wy += 30; selmon->wh -= 30;
	return 1;
}

static BspWorkspace *
mouse_setup(void)
{
	reset();
	add_client(selmon, 2, 1);
	focus(add_client(selmon, 1, 1));
	arrange(selmon);
	handler[DestroyNotify] = handler[UnmapNotify] = handler[MapRequest] = dispatch_event;
	handler[PropertyNotify] = handler[Expose] = dispatch_event;
	return bsp_active(selmon);
}
