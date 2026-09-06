static Client *
wintoclient(Window window)
{
	Monitor *m;
	Client *c;
	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			if (c->win == window)
				return c;
	return NULL;
}


static void
arrange(Monitor *m)
{
	++f.arrangements;
	if (m->lt[m->sellt]->arrange)
		m->lt[m->sellt]->arrange(m);
}

static Monitor *
add_monitor(int num)
{
	Monitor *m = calloc(1, sizeof(*m));
	assert(m);
	m->num = num; m->wx = num * 1000; m->wy = 20; m->ww = 1000; m->wh = 800;
	m->tagset[0] = 1; m->tagset[1] = 2;
	m->lt[0] = &bsp_layout; m->lt[1] = &floating_layout;
	m->next = mons; mons = m;
	return m;
}

static Client *
add_client(Monitor *m, Window window, unsigned int tagsmask)
{
	Client *c = calloc(1, sizeof(*c));
	assert(c);
	c->win = window; c->tags = tagsmask; c->bw = 2; c->cfact = 1;
	c->mon = m; c->next = m->clients; c->snext = m->stack;
	m->clients = m->stack = c;
	return c;
}

static void
remove_client(Window window)
{
	Client *c = wintoclient(window), **p;
	assert(c);
	for (p = &c->mon->clients; *p != c; p = &(*p)->next);
	*p = c->next;
	for (p = &c->mon->stack; *p != c; p = &(*p)->snext);
	*p = c->snext;
	if (c->mon->sel == c) c->mon->sel = NULL;
	bsp_forget_window(&bsp_forest, window);
	free(c);
}

static void
reset(void)
{
	Monitor *m;
	while ((m = mons)) {
		while (m->clients) remove_client(m->clients->win);
		bsp_forget_monitor(&bsp_forest, m->num);
		mons = m->next;
		free(m);
	}
	bsp_forest_clear(&bsp_forest);
	memset(&f, 0, sizeof(f));
	memset(handler, 0, sizeof(handler));
	f.grab_result = GrabSuccess; f.pointer_ok = 1;
	f.startx = 500; f.starty = 420;
	selmon = add_monitor(0);
}

static void focus(Client *c) { selmon = c->mon; c->mon->sel = c; bsp_focus_client(c); }
static void qs_publish(void) { ++f.publishes; }
static void updateclientlist(void) { ++f.updates; }
static Monitor *
recttomon(int x, int y, int w, int h)
{
	Monitor *m;
	(void)w; (void)h;
	for (m = mons; m; m = m->next)
		if (x >= m->wx && x < m->wx + m->ww && y >= m->wy && y < m->wy + m->wh)
			return m;
	return selmon;
}
