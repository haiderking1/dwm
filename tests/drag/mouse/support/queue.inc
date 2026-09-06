/* Queue exhaustion and drain limits turn a stuck event loop into a failure. */
static void
push_pointer(int type, unsigned int button, int x, int y, Time time,
             void (*before)(void))
{
	QueuedEvent *q;
	assert(f.queued < LIMIT);
	q = &f.queue[f.queued++];
	q->before = before;
	q->event.type = type;
	if (type == MotionNotify) {
		q->event.xmotion.window = root;
		q->event.xmotion.x_root = x;
		q->event.xmotion.y_root = y;
		q->event.xmotion.time = time;
	} else {
		assert(type == ButtonRelease || type == ButtonPress);
		q->event.xbutton.window = root;
		q->event.xbutton.button = button;
		q->event.xbutton.x_root = x;
		q->event.xbutton.y_root = y;
		q->event.xbutton.time = time;
	}
}

static XEvent *
push_dispatch(int type, Window window, void (*before)(void))
{
	QueuedEvent *q;
	assert(f.queued < LIMIT);
	q = &f.queue[f.queued++];
	q->before = before;
	q->event.type = type;
	switch (type) {
	case DestroyNotify: q->event.xdestroywindow.window = window; break;
	case ConfigureRequest: q->event.xconfigurerequest.window = window; break;
	case ConfigureNotify: q->event.xconfigure.window = window; break;
	case PropertyNotify: q->event.xproperty.window = window; break;
	case Expose: q->event.xexpose.window = window; break;
	default: assert(!"unsupported fixture event");
	}
	return &q->event;
}

int
XMaskEvent(Display *display, long mask, XEvent *event)
{
	QueuedEvent *q;
	const long needed = MOUSEMASK | ExposureMask | SubstructureRedirectMask
	                  | SubstructureNotifyMask | StructureNotifyMask | PropertyChangeMask;
	assert(display == dpy && f.grabbed);
	assert((mask & needed) == needed);
	assert(f.consumed < f.queued && "drag exhausted bounded event queue");
	q = &f.queue[f.consumed++];
	if (q->before)
		q->before();
	*event = q->event;
	return 0;
}

Bool
XCheckMaskEvent(Display *display, long mask, XEvent *event)
{
	assert(display == dpy && mask == EnterWindowMask && !f.grabbed);
	assert(f.syncs == 1);
	assert(++f.enter_checks < LIMIT && "unbounded EnterNotify drain");
	if (!f.enter_pending)
		return False;
	--f.enter_pending;
	memset(event, 0, sizeof(*event));
	event->type = EnterNotify;
	return True;
}
