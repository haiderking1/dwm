static Client *
nexttiled(Client *c)
{
	for (; c && (c->isfloating || !ISVISIBLE(c)); c = c->next);
	return c;
}

static Client *
wintoclient(Window w)
{
	Client *c;
	if (!f.mon) return NULL;
	for (c = f.mon->clients; c; c = c->next)
		if (c->win == w) return c;
	return NULL;
}

static void
resizeclient(Client *c, int x, int y, int w, int h)
{
	assert(c == wintoclient(c->win));
	assert(w > 0 && h > 0);
	c->x = x; c->y = y; c->w = w; c->h = h;
	++f.resizes;
}

static void
tile(Monitor *m)
{
	resize_tile(m);
}

static void
monocle(Monitor *m)
{
	(void)m;
}

static void
arrange(Monitor *m)
{
	assert(m == f.mon);
	++f.arrangements;
	if (m->lt[m->sellt]->arrange)
		m->lt[m->sellt]->arrange(m);
}

static Client *
add_client(unsigned int index)
{
	Client *c = calloc(1, sizeof(*c)), **link;
	assert(index < CAPACITY && !f.clients[index] && c);
	c->win = 100 + index;
	c->cfact = 1.0f;
	c->bw = 2;
	c->tags = 1;
	c->mon = f.mon;
	for (link = &f.mon->clients; *link; link = &(*link)->next);
	*link = c;
	f.clients[index] = c;
	return c;
}

static void
fixture_init(unsigned int count, int masters)
{
	unsigned int i;
	memset(&f, 0, sizeof(f));
	memset(handler, 0, sizeof(handler));
	f.mon = calloc(1, sizeof(*f.mon));
	assert(f.mon);
	f.mon->wx = -100; f.mon->wy = 20;
	f.mon->ww = 1000; f.mon->wh = 900;
	f.mon->mfact = 0.55f; f.mon->nmaster = masters;
	f.mon->tagset[0] = 1; f.mon->tagset[1] = 2;
	f.mon->lt[0] = &tiled_layout; f.mon->lt[1] = &floating_layout;
	for (i = 0; i < count; ++i) add_client(i);
	f.mon->sel = f.mon->clients;
	f.pointer_ok = 1; f.grab_result = GrabSuccess;
	f.startx = 200; f.starty = 100;
	tile(f.mon);
	f.resizes = 0;
	handler[Expose] = dispatch_event;
	handler[DestroyNotify] = dispatch_event;
	handler[UnmapNotify] = dispatch_event;
	handler[MapRequest] = dispatch_event;
	handler[PropertyNotify] = dispatch_event;
	handler[ConfigureNotify] = dispatch_event;
	handler[ConfigureRequest] = dispatch_event;
}

static void
remove_client(Window w)
{
	Client *c = wintoclient(w), **link;
	unsigned int i;
	assert(c);
	for (link = &f.mon->clients; *link != c; link = &(*link)->next) assert(*link);
	*link = c->next;
	if (f.mon->sel == c) f.mon->sel = f.mon->clients;
	for (i = 0; i < CAPACITY; ++i)
		if (f.clients[i] == c) f.clients[i] = NULL;
	free(c);
}

static void
fixture_dispose(void)
{
	unsigned int i;
	for (i = 0; i < CAPACITY; ++i) free(f.clients[i]);
	free(f.mon);
}
