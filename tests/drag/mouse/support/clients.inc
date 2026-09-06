static Client *
wintoclient(Window window)
{
	size_t m;
	for (m = 0; m < 2; ++m) {
		Client *c;
		size_t walked = 0;
		for (c = f.monitors[m].clients; c; c = c->next) {
			assert(++walked <= 2);
			if (c->win == window)
				return c;
		}
	}
	return NULL;
}

static ServerWindow *
server_window(Window window)
{
	size_t i;
	for (i = 0; i < 2; ++i)
		if (f.windows[i].win == window) {
			assert(f.windows[i].alive && "X request on destroyed window");
			return &f.windows[i];
		}
	assert(!"unknown window");
	return NULL;
}

static void
fixture_init(void)
{
	size_t i;
	memset(&f, 0, sizeof(f));
	memset(handler, 0, sizeof(handler));
	f.grab_result = GrabSuccess;
	f.pointer_ok = 1;
	f.startx = 40;
	f.starty = 70;
	f.enter_pending = 2;
	for (i = 0; i < 2; ++i) {
		Monitor *m = &f.monitors[i];
		m->num = (int)i;
		m->mx = 400 * (int)i;
		m->slotx = m->mx + 10;
		m->sloty = 40;
		m->slotw = 180;
		m->sloth = 140;
		m->step = 200;
		m->tagset[0] = 1u << i;
		m->lt[0] = &tiled_layout;
		m->lt[1] = &floating_layout;
	}
	f.source = calloc(1, sizeof(*f.source));
	f.target = calloc(1, sizeof(*f.target));
	assert(f.source && f.target);
	for (i = 0; i < 2; ++i) {
		Client *c = i ? f.target : f.source;
		c->win = i ? TARGET : SOURCE;
		c->x = 10 + 200 * (int)i;
		c->y = 40;
		c->w = 180;
		c->cfact = i ? 2.5f : 1.25f;
		c->h = 140 + (int)(8 * c->cfact);
		c->bw = 2;
		c->tags = 1;
		c->mon = &f.monitors[0];
		f.windows[i] = (ServerWindow){ c->win, c->x, c->y, 1 };
	}
	f.source->next = f.target;
	f.source->snext = f.target;
	f.monitors[0].clients = f.monitors[0].stack = f.monitors[0].sel = f.source;
	f.source_before = *f.source;
	f.target_before = *f.target;
	handler[DestroyNotify] = dispatch_event;
	handler[ConfigureRequest] = dispatch_event;
	handler[ConfigureNotify] = dispatch_event;
	handler[PropertyNotify] = dispatch_event;
	handler[Expose] = dispatch_event;
}

static void
fixture_dispose(void)
{
	free(f.source);
	free(f.target);
	f.source = f.target = NULL;
}

static void
destroy_client(Window window)
{
	Client *c = wintoclient(window), **link;
	Monitor *m;
	assert(c);
	m = c->mon;
	for (link = &m->clients; *link != c; link = &(*link)->next)
		assert(*link);
	*link = c->next;
	for (link = &m->stack; *link != c; link = &(*link)->snext)
		assert(*link);
	*link = c->snext;
	if (m->sel == c)
		m->sel = m->stack;
	server_window(window)->alive = 0;
	if (window == SOURCE)
		f.source = NULL;
	else
		f.target = NULL;
	f.destroyed = window;
	free(c); /* ASan must catch any pointer retained across dispatch. */
}
