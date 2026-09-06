/* Like resize(), unchanged logical geometry sends no X geometry request.
 * This deliberately cannot repair a preview by arranging unchanged tiles. */
static void
tile_fixture(Monitor *mon)
{
	Client *c;
	int slot = 0;
	for (c = mon->clients; c; c = c->next) {
		int x;
		assert(slot < 2);
		if (!ISVISIBLE(c) || c->isfloating || c->isfullscreen)
			continue;
		x = mon->slotx + slot++ * mon->step;
		if (c->x == x && c->y == mon->sloty
		&& c->w == mon->slotw && c->h == mon->sloth)
			continue;
		c->x = x;
		c->y = mon->sloty;
		c->w = mon->slotw;
		c->h = mon->sloth;
		++f.resizes;
		XMoveWindow(dpy, c->win, c->x, c->y);
	}
}

static void
arrange(Monitor *mon)
{
	assert(mon == &f.monitors[0] || mon == &f.monitors[1]);
	assert(f.arrangements < LIMIT);
	f.arranged[f.arrangements++] = mon;
	if (mon->lt[mon->sellt]->arrange)
		mon->lt[mon->sellt]->arrange(mon);
}

static void
focus(Client *c)
{
	Client **link;
	assert(c && c == wintoclient(c->win) && !f.grabbed);
	++f.focuses;
	f.focused = c->win;
	c->mon->sel = c;
	/* dwm focus moves the selected client to the head of the stack. */
	for (link = &c->mon->stack; *link != c; link = &(*link)->snext)
		assert(*link);
	*link = c->snext;
	c->snext = c->mon->stack;
	c->mon->stack = c;
}

static Monitor *
recttomon(int x, int y, int w, int h)
{
	assert(w == 1 && h == 1);
	assert(y >= 0 && y < 300);
	assert(x >= 0 && x < 800);
	return &f.monitors[x / 400];
}

static int
getrootptr(int *x, int *y)
{
	assert(f.grabbed);
	++f.queries;
	if (!f.pointer_ok)
		return 0;
	*x = f.startx;
	*y = f.starty;
	return 1;
}

static void
updateclientlist(void)
{
	assert(!f.grabbed);
	++f.updates;
}

static void
qs_publish(void)
{
	++f.publishes;
}
