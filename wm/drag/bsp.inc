/* A previously migrated client can still appear in a dormant source view.
 * Prune before exchanging lists, while membership still describes old slots,
 * so replace cannot collide with a stale copy of the incoming identity. */
static int
bsp_drag_prepare(Client *a, Client *b)
{
	BspWorkspace *view;
	Monitor *m;

	if (!a || !b)
		return 0;
	for (view = bsp_forest.views; view; view = view->next) {
		m = view->monitor == a->mon->num ? a->mon :
		    view->monitor == b->mon->num ? b->mon : NULL;
		if (m && !bsp_sync_view(view, m, 0))
			return 0;
	}
	if (a->mon->lt[a->mon->sellt]->arrange == dwindle && !bsp_active(a->mon))
		return 0;
	if (b->mon != a->mon && b->mon->lt[b->mon->sellt]->arrange == dwindle
	&& !bsp_active(b->mon))
		return 0;
	return 1;
}

/* Called only after the pure list swap succeeds, before focus/arrange. The
 * monitor pointers describe the original slots, not the clients' new homes. */
static void
bsp_drag_swap(Client *a, Client *b, Monitor *am, Monitor *bm)
{
	BspWorkspace *view;

	if (am == bm) {
		if (am->lt[am->sellt]->arrange == dwindle
		&& (view = bsp_workspace(&bsp_forest, am->num,
		                        am->tagset[am->seltags] & TAGMASK, 0)))
			bsp_tree_swap(&view->tree, a->win, b->win);
		return;
	}
	/* Even a currently weighted/floating monitor may own dormant BSP views. */
	for (view = bsp_forest.views; view; view = view->next) {
		if (view->monitor == am->num)
			bsp_tree_replace(&view->tree, a->win, b->win);
		else if (view->monitor == bm->num)
			bsp_tree_replace(&view->tree, b->win, a->win);
	}
}
