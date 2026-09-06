static WeightFixture *restore_fixture;
static WeightFixture expected_clients;
static BspForest expected_forest;
static unsigned int apply_checks, arrange_checks;

static void
assert_nodes(const BspNode *actual, const BspNode *expected, const BspNode *parent)
{
	assert(!!actual == !!expected);
	if (!expected) return;
	assert(actual->parent == parent);
	assert(actual->window == expected->window);
	if (!expected->window) {
		assert(actual->axis == expected->axis && actual->ratio == expected->ratio);
		assert_nodes(actual->child[0], expected->child[0], actual);
		assert_nodes(actual->child[1], expected->child[1], actual);
	}
}

static void
assert_trees(BspForest *actual, const BspForest *expected)
{
	const BspWorkspace *view;
	BspWorkspace *found;
	unsigned int count = 0, expected_count = 0;
	for (view = actual->views; view; view = view->next) count++;
	for (view = expected->views; view; view = view->next) {
		expected_count++;
		found = bsp_workspace(actual, view->monitor, view->tags, 0);
		assert(found && found->tree.focus == view->tree.focus);
		assert_nodes(found->tree.root, view->tree.root, NULL);
	}
	assert(count == expected_count);
}

static void
check_apply_order(const BspForest *parsed)
{
	unsigned int i;
	assert(parsed->views && !forest_applies && !arrange_checks);
	assert(selmon == &restore_fixture->monitors[1]);
	for (i = 0; i < LENGTH(restore_fixture->monitors); i++) {
		Monitor *m = &restore_fixture->monitors[i];
		const Monitor *saved = &expected_clients.monitors[i];
		assert(m->clients == saved->clients && m->stack == saved->stack);
		assert(m->sel == saved->sel && m->nmaster == saved->nmaster);
		assert(m->mfact == saved->mfact && m->seltags == saved->seltags);
		assert(m->showbar == saved->showbar && m->topbar == saved->topbar);
		assert(m->sellt == saved->sellt);
		assert(memcmp(m->tagset, saved->tagset, sizeof m->tagset) == 0);
		assert(memcmp(m->lt, saved->lt, sizeof m->lt) == 0);
	}
	for (i = 0; i < LENGTH(restore_fixture->clients); i++) {
		Client *c = &restore_fixture->clients[i];
		const Client *saved = &expected_clients.clients[i];
		assert(c->mon == saved->mon && c->tags == saved->tags);
		assert(c->cfact == saved->cfact && c->next == saved->next);
		assert(c->snext == saved->snext && c->isfloating == saved->isfloating);
		assert(c->isfullscreen == saved->isfullscreen && c->oldstate == saved->oldstate);
		assert(c->x == saved->x && c->y == saved->y);
		assert(c->w == saved->w && c->h == saved->h);
		assert(c->oldx == saved->oldx && c->oldy == saved->oldy);
		assert(c->oldw == saved->oldw && c->oldh == saved->oldh);
	}
	apply_checks++;
}

static void
check_arrange_order(void)
{
	assert(apply_checks == 1 && forest_applies == 1);
	assert_trees(&checkpoint_forest, &expected_forest);
	arrange_checks++;
}

static void
test_forest_roundtrip(void)
{
	WeightFixture fixture;
	FILE *file, *serialized;
	forest_fixture_init(&fixture);
	populate_forest(&expected_forest);
	assert_trees(&checkpoint_forest, &expected_forest);
	file = weight_checkpoint();
	serialized = forest_snapshot(&checkpoint_forest);
	memcpy(&expected_clients, &fixture, sizeof fixture);
	simulate_scan(&fixture);
	restore_fixture = &fixture;
	apply_checks = arrange_checks = 0;
	checkpoint_before_apply = check_apply_order;
	checkpoint_before_arrange = check_arrange_order;
	assert(restore_checkpoint(file));
	assert(apply_checks == 1 && arrange_checks == 1);
	assert_forest_snapshot(serialized, &checkpoint_forest);
	assert(fclose(serialized) == 0);
	checkpoint_before_apply = NULL;
	checkpoint_before_arrange = NULL;
	bsp_forest_clear(&expected_forest);
	bsp_forest_clear(&checkpoint_forest);
}

static void
test_empty_forest_roundtrip(void)
{
	WeightFixture fixture;
	FILE *file;
	forest_fixture_init(&fixture);
	bsp_forest_clear(&checkpoint_forest);
	file = weight_checkpoint();
	simulate_scan(&fixture);
	assert(checkpoint_forest.views);
	assert(restore_checkpoint(file));
	assert(!checkpoint_forest.views && forest_applies == 1);
	assert(selmon == &fixture.monitors[1]);
	assert(mons->clients == &fixture.clients[3]);
	assert(mons->sel == &fixture.clients[1]);
	assert(fixture.clients[0].cfact == weights[0]);
	assert(fixture.clients[2].mon == selmon && fixture.clients[2].tags == 16);
}
