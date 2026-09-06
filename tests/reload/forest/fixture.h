#ifndef RELOAD_FOREST_FIXTURE_H
#define RELOAD_FOREST_FIXTURE_H
#include "../checkpoint/fixture.h"

static void
populate_forest(BspForest *forest)
{
	const uint64_t windows[] = {30, 10, 40};
	const uint64_t reverse[] = {40, 30, 10};
	const uint64_t remote[] = {20};
	const BspRect bounds = {0, 20, 1920, 1060};
	BspWorkspace *active, *hidden, *other, *combined;
	BspNode *branch;
	bsp_forest_clear(forest);
	active = bsp_workspace(forest, 0, 4, 1);
	hidden = bsp_workspace(forest, 0, 2, 1);
	other = bsp_workspace(forest, 1, 16, 1);
	combined = bsp_workspace(forest, 0, 6, 1);
	assert(active && hidden && other && combined);
	assert(bsp_tree_sync(&active->tree, windows, LENGTH(windows), 0, bounds));
	assert(bsp_tree_sync(&hidden->tree, reverse, LENGTH(reverse), 0, bounds));
	assert(bsp_tree_sync(&other->tree, remote, LENGTH(remote), 0, bounds));
	assert(bsp_tree_sync(&combined->tree, windows, LENGTH(windows), 0, bounds));
	/* Public node fields let the fixture specify exact persisted splits. */
	active->tree.root->axis = BSP_HORIZONTAL;
	active->tree.root->ratio = .31;
	branch = active->tree.root->child[0]->window ?
	         active->tree.root->child[1] : active->tree.root->child[0];
	assert(branch && !branch->window);
	branch->axis = BSP_VERTICAL;
	branch->ratio = .67;
	hidden->tree.root->axis = BSP_VERTICAL;
	hidden->tree.root->ratio = .73;
	combined->tree.root->axis = BSP_HORIZONTAL;
	combined->tree.root->ratio = .44;
	assert(bsp_tree_swap(&active->tree, 30, 40));
	assert(bsp_tree_swap(&hidden->tree, 30, 10));
	assert(bsp_tree_focus(&active->tree, 10));
	assert(bsp_tree_focus(&hidden->tree, 30));
	assert(bsp_tree_focus(&other->tree, 20));
	assert(bsp_tree_focus(&combined->tree, 40));
}

static void
forest_fixture_init(WeightFixture *fixture)
{
	unsigned int i;
	weight_fixture_init(fixture);
	attach(&fixture->clients[3]);
	attachstack(&fixture->clients[3]);
	for (i = 0; i < LENGTH(fixture->clients); i++) {
		fixture->clients[i].isfloating = 0;
		fixture->clients[i].bw = borderpx;
		if (fixture->clients[i].mon == mons) fixture->clients[i].tags = 6;
	}
	populate_forest(&checkpoint_forest);
}

static void
replace_live_forest(void)
{
	const uint64_t window = 40;
	BspWorkspace *view;
	bsp_forest_clear(&checkpoint_forest);
	view = bsp_workspace(&checkpoint_forest, 0, 1, 1);
	assert(view);
	assert(bsp_tree_sync(&view->tree, &window, 1, 0, (BspRect){0, 0, 640, 480}));
}

static void
simulate_scan(WeightFixture *fixture)
{
	unsigned int i;
	for (i = 0; i < LENGTH(fixture->monitors); i++) {
		Monitor *m = &fixture->monitors[i];
		m->clients = m->stack = m->sel = NULL;
		m->tagset[0] = m->tagset[1] = 1;
		m->seltags = m->sellt = 0;
		m->lt[0] = m->lt[1] = &layouts[0];
		m->mfact = .8f; m->nmaster = 5;
		m->showbar = m->topbar = 0;
	}
	selmon = mons;
	for (i = 0; i < LENGTH(fixture->clients); i++) {
		Client *c = &fixture->clients[i];
		c->mon = mons;
		c->tags = 1; c->cfact = 9.0f;
		c->x = c->y = -1000;
		c->w = c->h = 100;
		attach(c); attachstack(c);
	}
	replace_live_forest();
}

static long
forest_offset(FILE *file)
{
	ReloadHeader header;
	rewind(file);
	assert(fread(&header, sizeof header, 1, file) == 1);
	assert(header.version == 3);
	return sizeof header + header.monitors * sizeof(ReloadMonitor) +
	       header.clients * (sizeof(ReloadClient) + sizeof(float));
}
#endif
