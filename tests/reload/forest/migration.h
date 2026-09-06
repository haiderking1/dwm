static unsigned int legacy_arranges;

static void
seed_legacy_views(void)
{
	Monitor *m;
	Client *c;
	BspWorkspace *view;
	uint64_t windows[4];
	size_t count;
	assert(!checkpoint_forest.views && forest_applies == 1);
	for (m = mons; m; m = m->next) {
		view = bsp_workspace(&checkpoint_forest, m->num, m->tagset[m->seltags], 1);
		assert(view);
		count = 0;
		for (c = m->clients; c; c = c->next)
			if ((c->tags & view->tags) &&
			    (!c->isfloating || (c->isfullscreen && !c->oldstate))) {
				assert(count < LENGTH(windows));
				windows[count++] = c->win;
				assert(bsp_tree_sync(&view->tree, windows, count,
				                     count > 1 ? windows[count - 2] : 0,
				                     (BspRect){m->wx, m->by, m->ww, m->mh}));
			}
	}
	legacy_arranges++;
}

static void
test_legacy_forest_migration(unsigned int version)
{
	WeightFixture fixture;
	BspWorkspace *view;
	FILE *source, *bad, *good;
	unsigned int i;
	weight_fixture_init(&fixture);
	populate_forest(&checkpoint_forest);
	source = legacy_checkpoint(version);
	if (version == 2) assert(fwrite(weights, sizeof weights, 1, source) == 1);
	assert(ftell(source) == legacy_size() + (version == 2 ? (long)sizeof weights : 0));
	bad = checkpoint_prefix(source, checkpoint_length(source) - 1);
	change_live_state(&fixture);
	fixture.clients[3].tags = 4;
	assert_rejected_unchanged(bad, &fixture);
	good = checkpoint_prefix(source, checkpoint_length(source));
	legacy_arranges = 0;
	checkpoint_before_arrange = seed_legacy_views;
	assert(restore_checkpoint(good));
	assert(legacy_arranges == 1 && forest_applies == 1);
	for (i = 0; i < LENGTH(legacy_clients); i++)
		if (i != 1) assert(fixture.clients[i].cfact == (version == 1 ? 1.0f : weights[i]));
	assert(fixture.clients[1].cfact == 9.0f && fixture.clients[3].cfact == 1.0f);
	assert(selmon == &fixture.monitors[1] && selmon->sel == &fixture.clients[2]);
	assert(mons->tagset[0] == 4 && selmon->tagset[1] == 16);
	/* The saved tiled window closed. Layout seeds the newcomer, not old leaves. */
	view = bsp_workspace(&checkpoint_forest, 0, 4, 0);
	assert(view && view->tree.root && view->tree.root->window == 40);
	assert(!bsp_workspace(&checkpoint_forest, 0, 2, 0));
	assert(!bsp_workspace(&checkpoint_forest, 0, 6, 0));
	assert(!bsp_workspace(&checkpoint_forest, 0, 1, 0));
	checkpoint_before_arrange = NULL;
	assert(fclose(source) == 0);
	bsp_forest_clear(&checkpoint_forest);
}
