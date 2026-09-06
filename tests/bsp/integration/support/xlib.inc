int
XGrabPointer(Display *display, Window window, Bool owner, unsigned int mask,
             int pointermode, int keyboardmode, Window confine, Cursor shape, Time time)
{
	assert(display == dpy && window == root && !owner);
	assert(mask == MOUSEMASK && pointermode == GrabModeAsync && keyboardmode == GrabModeAsync);
	assert(confine == None && (shape == resize_cursor.cursor || shape == move_cursor.cursor) && time == CurrentTime);
	assert(!f.grabbed);
	++f.grabs;
	f.grabbed = f.grab_result == GrabSuccess;
	return f.grab_result;
}

int
XUngrabPointer(Display *display, Time time)
{
	assert(display == dpy && time == CurrentTime && f.grabbed);
	f.grabbed = 0; ++f.ungrabs;
	return 1;
}

static int
getrootptr(int *x, int *y)
{
	assert(f.grabbed);
	++f.queries;
	*x = f.startx; *y = f.starty;
	return f.pointer_ok;
}

int
XMaskEvent(Display *display, long mask, XEvent *ev)
{
	const long required = MOUSEMASK | ExposureMask | SubstructureRedirectMask
	                    | SubstructureNotifyMask | StructureNotifyMask | PropertyChangeMask;
	assert(display == dpy && f.grabbed && (mask & required) == required);
	assert(f.consumed < f.queued && "mouse loop failed to terminate");
	*ev = f.queue[f.consumed++];
	return 1;
}

int
XSync(Display *display, Bool discard)
{
	assert(display == dpy && !discard && !f.grabbed);
	++f.syncs;
	return 1;
}

int
XFlush(Display *display)
{
	assert(display == dpy);
	++f.flushes;
	return 1;
}

Bool
XCheckMaskEvent(Display *display, long mask, XEvent *ev)
{
	(void)ev;
	assert(display == dpy && mask == EnterWindowMask && !f.grabbed);
	++f.enter_checks;
	return False;
}

int
XMoveWindow(Display *display, Window window, int x, int y)
{
	(void)x; (void)y;
	assert(display == dpy && wintoclient(window));
	++f.moves;
	return 1;
}

int
XRaiseWindow(Display *display, Window window)
{
	assert(display == dpy && wintoclient(window));
	++f.raises;
	return 1;
}
