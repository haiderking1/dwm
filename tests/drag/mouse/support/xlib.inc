int
XGrabPointer(Display *display, Window window, Bool owner_events,
             unsigned int mask, int pointer_mode, int keyboard_mode,
             Window confine_to, Cursor pointer_cursor, Time time)
{
	assert(display == dpy && window == root && owner_events == False);
	assert(mask == MOUSEMASK);
	assert(pointer_mode == GrabModeAsync && keyboard_mode == GrabModeAsync);
	assert(confine_to == None && pointer_cursor == move_cursor.cursor);
	assert(time == CurrentTime && !f.grabbed);
	++f.grabs;
	f.grabbed = f.grab_result == GrabSuccess;
	return f.grab_result;
}

int
XUngrabPointer(Display *display, Time time)
{
	assert(display == dpy && time == CurrentTime && f.grabbed);
	f.grabbed = 0;
	++f.ungrabs;
	return 0;
}

int
XMoveWindow(Display *display, Window window, int x, int y)
{
	ServerWindow *w = server_window(window);
	assert(display == dpy && wintoclient(window));
	assert(f.nmoves < LIMIT);
	f.moves[f.nmoves++] = (Move){ window, x, y };
	w->x = x;
	w->y = y;
	return 0;
}

int
XRaiseWindow(Display *display, Window window)
{
	assert(display == dpy && window == SOURCE && f.grabbed);
	assert(wintoclient(window));
	++f.raises;
	return 0;
}

int
XFlush(Display *display)
{
	assert(display == dpy && f.grabbed);
	++f.flushes;
	return 0;
}

int
XSync(Display *display, Bool discard)
{
	assert(display == dpy && discard == False && !f.grabbed);
	++f.syncs;
	return 0;
}
