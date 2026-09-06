/* Prune against real clients without activating or inventing dormant views. */
static int
bsp_clean_forest(BspForest *forest)
{
	BspWorkspace *view = forest->views, *next;
	Monitor *m;

	while (view) {
		next = view->next;
		for (m = mons; m && m->num != view->monitor; m = m->next);
		if (!m) {
			bsp_forget_monitor(forest, view->monitor);
			/* forget_monitor can remove more than this one workspace. */
			view = forest->views;
			continue;
		}
		if (!bsp_sync_view(view, m, 0))
			return 0;
		view = next;
	}
	return 1;
}

static int
bsp_checkpoint_write(FILE *stream)
{
	return bsp_clean_forest(&bsp_forest) && bsp_forest_write(stream, &bsp_forest);
}

static int
bsp_checkpoint_read(FILE *stream, BspForest *out)
{
	BspForest temporary = {0};
	BspWorkspace *view, *other;

	if (!bsp_forest_read(stream, &temporary))
		return 0;
	for (view = temporary.views; view; view = view->next) {
		if (view->monitor < 0 || !view->tags || (view->tags & ~TAGMASK))
			goto invalid;
		for (other = view->next; other; other = other->next)
			if (other->monitor == view->monitor && other->tags == view->tags)
				goto invalid;
	}
	bsp_forest_clear(out);
	*out = temporary;
	return 1;
invalid:
	bsp_forest_clear(&temporary);
	return 0;
}

static void
bsp_checkpoint_apply(BspForest *temporary)
{
	bsp_forest_clear(&bsp_forest);
	bsp_forest = *temporary;
	temporary->views = NULL;
	/* Allocation failure must not leave stale window identities live. The
	 * ordinary layout path can reseed from restored client order next time. */
	if (!bsp_clean_forest(&bsp_forest))
		bsp_forest_clear(&bsp_forest);
}
