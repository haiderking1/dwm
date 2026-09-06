enum {
	ChangeMonocle = 1, ChangeFloating, ChangeTags, ChangeMaster,
	ChangeFactor, ChangeBorder, ChangeWeight, ChangeOrder, ChangeFloat,
	ChangeFullscreen, ChangeFocus, ChangeWorkarea, ChangeMonitor,
	ChangeDock, ChangeGeometry, ChangeView, ChangeNeighbor, ChangeCount
};

static XEvent *
push_event(int type)
{
	XEvent *ev;
	assert(f.queued < QUEUE_SIZE);
	ev = &f.queue[f.queued++];
	memset(ev, 0, sizeof(*ev));
	ev->type = type;
	return ev;
}

static void
push_pointer(int type, unsigned int button, int x, int y, Time time)
{
	XEvent *ev = push_event(type);
	if (type == MotionNotify) {
		ev->xmotion.x_root = x; ev->xmotion.y_root = y;
		ev->xmotion.x = -9999; ev->xmotion.y = -9999;
		ev->xmotion.time = time;
	} else {
		ev->xbutton.x_root = x; ev->xbutton.y_root = y;
		ev->xbutton.x = -9999; ev->xbutton.y = -9999;
		ev->xbutton.button = button; ev->xbutton.time = time;
	}
}

static void
qs_publish(void)
{
	++f.publishes;
}

static int
qs_dock_event(XEvent *ev)
{
	if (ev->type != PropertyNotify || ev->xproperty.atom != ChangeDock)
		return 0;
	++f.docks;
	f.mon->wy += 40; f.mon->wh -= 40;
	arrange(f.mon);
	return 1;
}

static void
dispatch_event(XEvent *ev)
{
	Client *c = f.mon->sel, *other;
	Monitor *replacement;
	++f.dispatches;
	switch (ev->type) {
	case DestroyNotify: remove_client(ev->xdestroywindow.window); break;
	case UnmapNotify: remove_client(ev->xunmap.window); break;
	case MapRequest: add_client(15); arrange(f.mon); break;
	case ConfigureNotify: f.mon->ww -= 100; arrange(f.mon); break;
	case ConfigureRequest: break; /* Tiled requests are acknowledged only. */
	case Expose: break;
	case PropertyNotify:
		switch (ev->xproperty.atom) {
		case ChangeMonocle: f.mon->lt[0] = &monocle_layout; break;
		case ChangeFloating: f.mon->sellt = 1; break;
		case ChangeTags: f.mon->tagset[0] = 2; break;
		case ChangeMaster: ++f.mon->nmaster; break;
		case ChangeFactor: f.mon->mfact = 0.7f; break;
		case ChangeBorder: ++c->bw; break;
		case ChangeWeight: c->cfact = 3.5f; break;
		case ChangeOrder:
			other = c->next; c->next = other->next;
			other->next = c; f.mon->clients = other;
			break;
		case ChangeFloat: c->isfloating = 1; break;
		case ChangeFullscreen: c->isfullscreen = 1; break;
		case ChangeFocus: f.mon->sel = c->next; break;
		case ChangeWorkarea: f.mon->wh -= 10; break;
		case ChangeMonitor:
			replacement = malloc(sizeof(*replacement)); assert(replacement);
			*replacement = *f.mon;
			free(f.mon); f.mon = replacement;
			for (other = f.mon->clients; other; other = other->next)
				other->mon = replacement;
			break;
		case ChangeGeometry: ++c->y; break;
		case ChangeView: f.mon->seltags = 1; break;
		case ChangeNeighbor: c->next->isfloating = 1; break;
		default: assert(!"unknown change");
		}
		break;
	default: assert(!"unexpected event");
	}
}
