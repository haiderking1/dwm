/* Test the real async request helper rather than a substitute resize routine. */
int XConfigureWindow(Display *display, Window window, unsigned int mask,
                     XWindowChanges *changes)
{
    Client *c = wintoclient(window);
    assert(display == dpy && c && !c->isfullscreen);
    assert(mask == (CWX | CWY | CWWidth | CWHeight | CWBorderWidth));
    assert(changes->x == c->x && changes->y == c->y);
    assert(changes->width == c->w && changes->height == c->h);
    assert(changes->border_width == c->bw && c->w > 0 && c->h > 0);
    ++f.resizes;
    return 1;
}
static void configure(Client *c)
{
    assert(wintoclient(c->win) == c);
    ++f.notifies;
}
