/* FIFO Xlib model: peek cannot skip lifecycle or release barriers. */
int XEventsQueued(Display *display, int mode)
{
    assert(display == dpy && mode == QueuedAfterReading);
    return (int)(f.queued - f.consumed);
}
int XPeekEvent(Display *display, XEvent *event)
{
    assert(display == dpy && f.consumed < f.queued);
    *event = f.queue[f.consumed];
    return 1;
}
int XNextEvent(Display *display, XEvent *event)
{
    assert(display == dpy && f.consumed < f.queued);
    *event = f.queue[f.consumed++];
    ++f.coalesced;
    return 1;
}
