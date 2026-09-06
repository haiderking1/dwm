/* Never search past a lifecycle or release barrier for newer motion.
 * QueuedAfterReading reads available input without flushing or blocking. */
static void
mouse_coalesce_motion(XEvent *event)
{
	XEvent next;
	unsigned int remaining = 256;

	if (event->type != MotionNotify)
		return;
	/* Bound work even if input arrives continuously. */
	while (remaining-- && XEventsQueued(dpy, QueuedAfterReading) > 0) {
		XPeekEvent(dpy, &next);
		if (next.type != MotionNotify)
			break;
		XNextEvent(dpy, event);
	}
}

/* Client ConfigureNotify echoes geometry already in Client. Root notifications
 * can change monitor bounds. Be conservative for other dispatched events. */
static int
mouse_reconcile_event(const XEvent *event, int dock)
{
	if (dock)
		return 1;
	switch (event->type) {
	case MotionNotify: case ButtonPress: case ButtonRelease:
	case Expose: case GraphicsExpose: case NoExpose:
		return 0;
	case ConfigureNotify:
		return event->xconfigure.window == root;
	default:
		return 1;
	}
}

/* Bar IPC contains no client geometry. Gesture cleanup publishes separately. */
static int
mouse_publish_event(const XEvent *event, int dock)
{
	return (dock || event->type != ConfigureRequest)
	    && mouse_reconcile_event(event, dock);
}
