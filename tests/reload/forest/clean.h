#ifndef RELOAD_FOREST_CLEAN_H
#define RELOAD_FOREST_CLEAN_H
/* Same dormant-view rule as integration: remove stale members, never seed. */
static int
checkpoint_clean_forest(BspForest *forest)
{
	BspWorkspace *view = forest->views;
	Monitor *m;
	Client *c;
	uint64_t *windows;
	size_t count, i;
	int ok, minimum;
	BspRect bounds;
	while (view) {
		for (m = mons; m && m->num != view->monitor; m = m->next);
		if (!m) {
			bsp_forget_monitor(forest, view->monitor);
			view = forest->views;
			continue;
		}
		count = 0;
		for (c = m->clients; c; c = c->next)
			if ((c->tags & view->tags) &&
			    (!c->isfloating || (c->isfullscreen && !c->oldstate)) &&
			    bsp_find(&view->tree, c->win)) count++;
		windows = count ? calloc(count, sizeof *windows) : NULL;
		if (count && !windows) return 0;
		i = 0;
		minimum = 1;
		for (c = m->clients; c; c = c->next)
			if ((c->tags & view->tags) &&
			    (!c->isfloating || (c->isfullscreen && !c->oldstate)) &&
			    bsp_find(&view->tree, c->win)) {
				windows[i++] = c->win;
				minimum = MAX(minimum, 2 * c->bw + 1);
			}
		bounds = (BspRect){m->wx, m->wy, MAX(1, m->ww), MAX(1, m->wh)};
		ok = bsp_tree_sync(&view->tree, windows, count, view->tree.focus, bounds);
		free(windows);
		if (!ok) return 0;
		bsp_tree_layout(&view->tree, bounds, minimum);
		view = view->next;
	}
	return 1;
}
#endif
