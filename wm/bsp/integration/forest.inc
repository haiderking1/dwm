/* Workspace keys use the full view mask, not a single tag index. */
static BspForest bsp_forest;

static BspRect
bsp_monitor_bounds(Monitor *m)
{
	return (BspRect){ m->wx, m->wy, MAX(1, m->ww), MAX(1, m->wh) };
}

static int
bsp_member(Client *c, unsigned int mask)
{
	return (c->tags & mask) && (!c->isfloating || (c->isfullscreen && !c->oldstate));
}

/* Dormant views lose stale leaves but acquire new leaves only on activation.
 * Never use mon->sel as the insertion anchor: manage() has already focused the
 * new client by the time arrange() runs. */
static int
bsp_sync_view(BspWorkspace *view, Monitor *m, int insert)
{
	Client *c;
	uint64_t *windows;
	size_t count = 0, i = 0;
	int minimum = 32, result;

	for (c = m->clients; c; c = c->next)
		if (bsp_member(c, view->tags) && (insert || bsp_find(&view->tree, c->win)))
			count++;
	windows = count ? calloc(count, sizeof(*windows)) : NULL;
	if (count && !windows)
		return 0;
	for (c = m->clients; c; c = c->next)
		if (bsp_member(c, view->tags) && (insert || bsp_find(&view->tree, c->win))) {
			windows[i++] = c->win;
			minimum = MAX(minimum, 2 * c->bw + 1);
		}
	if (!view->tree.root && count) {
		/* V2 has no forest: walk its restored client order into a dwindle
		 * chain, splitting the last seeded leaf rather than the first. */
		result = 1;
		for (i = 0; i < count && result; i++)
			result = bsp_tree_sync(&view->tree, windows, i + 1,
			                       i ? windows[i - 1] : 0, bsp_monitor_bounds(m));
	} else
		result = bsp_tree_sync(&view->tree, windows, count, view->tree.focus,
		                       bsp_monitor_bounds(m));
	free(windows);
	if (result)
		bsp_tree_layout(&view->tree, bsp_monitor_bounds(m), minimum);
	return result;
}

static BspWorkspace *
bsp_active(Monitor *m)
{
	BspWorkspace *view = bsp_workspace(&bsp_forest, m->num,
	                                  m->tagset[m->seltags] & TAGMASK, 1);
	return view && bsp_sync_view(view, m, 1) ? view : NULL;
}

static void
bsp_focus_client(Client *c)
{
	BspWorkspace *view;

	if (c && bsp_member(c, c->mon->tagset[c->mon->seltags])
	&& (view = bsp_workspace(&bsp_forest, c->mon->num,
	                              c->mon->tagset[c->mon->seltags] & TAGMASK, 0)))
		bsp_tree_focus(&view->tree, c->win);
}

static void
bsp_place_leaf(uint64_t window, BspRect rect, void *context)
{
	Monitor *m = context;
	Client *c;
	int width, height;

	for (c = m->clients; c && c->win != window; c = c->next);
	if (c && ISVISIBLE(c) && !c->isfloating && !c->isfullscreen) {
		width = MAX(1, rect.width - 2 * c->bw);
		height = MAX(1, rect.height - 2 * c->bw);
		if (c->x != rect.x || c->y != rect.y || c->w != width || c->h != height)
			resizeclient(c, rect.x, rect.y, width, height);
	}
}

static void
dwindle(Monitor *m)
{
	BspWorkspace *view = bsp_active(m);

	if (!view) {
		/* Keep every client arranged if allocation or a tree limit fails. */
		tile(m);
		return;
	}
	bsp_tree_visit(&view->tree, bsp_place_leaf, m);
	bsp_focus_client(m->sel);
}
