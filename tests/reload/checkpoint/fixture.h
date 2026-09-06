#include "../state_fixture.h"
#include "legacy.h"
#include "../forest/snapshot.h"

typedef struct {
	Monitor monitors[2];
	Client clients[4];
} WeightFixture;

static const float weights[] = { .125f, 4.0f, 32.0f };

static void
weight_fixture_init(WeightFixture *fixture)
{
	unsigned int i;
	bsp_forest_clear(&checkpoint_forest);
	checkpoint_before_apply = NULL;
	checkpoint_before_arrange = NULL;
	forest_applies = 0;
	memset(fixture, 0, sizeof *fixture);
	mons = fixture->monitors;
	selmon = &fixture->monitors[1];
	mons->next = selmon;
	for (i = 0; i < LENGTH(legacy_monitors); i++) {
		Monitor *m = &fixture->monitors[i];
		const LegacyMonitor *saved = &legacy_monitors[i];
		m->num = saved->num;
		m->nmaster = saved->nmaster;
		m->showbar = saved->showbar;
		m->topbar = saved->topbar;
		m->seltags = saved->seltags;
		m->sellt = saved->sellt;
		m->mfact = saved->mfact;
		memcpy(m->tagset, saved->tagset, sizeof m->tagset);
		m->lt[0] = &layouts[saved->layout[0]];
		m->lt[1] = &layouts[saved->layout[1]];
		m->mx = m->wx = i * 1920; m->mw = m->ww = 1920; m->mh = m->wh = 1080;
	}
	for (i = 0; i < LENGTH(legacy_clients); i++) {
		Client *c = &fixture->clients[i];
		const LegacyClient *saved = &legacy_clients[i];
		c->win = saved->window;
		c->mon = &fixture->monitors[saved->monitor];
		c->x = saved->x; c->y = saved->y; c->w = saved->w; c->h = saved->h;
		c->oldx = saved->oldx; c->oldy = saved->oldy;
		c->oldw = saved->oldw; c->oldh = saved->oldh;
		c->isfloating = saved->floating;
		c->isfullscreen = saved->fullscreen;
		c->oldstate = saved->oldstate;
		c->tags = saved->tags;
		c->cfact = weights[i];
		attachstack(c);
	}
	for (i = LENGTH(legacy_clients); i > 0; ) attach(&fixture->clients[--i]);
	fixture->monitors[0].sel = &fixture->clients[1];
	fixture->monitors[1].sel = &fixture->clients[2];
	fixture->clients[3].win = 40;
	fixture->clients[3].mon = mons;
	fixture->clients[3].w = 600; fixture->clients[3].h = 400;
	fixture->clients[3].tags = 1;
	fixture->clients[3].cfact = 1.0f;
}

static FILE *
weight_checkpoint(void)
{
	int fd = reload_state_save();
	FILE *file;
	assert(fd >= 0);
	file = fdopen(fd, "r+b");
	assert(file);
	return file;
}

static int
restore_checkpoint(FILE *file)
{
	int fd;
	assert(fflush(file) == 0);
	assert(fseek(file, 0, SEEK_SET) == 0);
	fd = dup(fileno(file));
	assert(fd >= 0);
	assert(fclose(file) == 0);
	return reload_state_restore(fd);
}

static void
change_live_state(WeightFixture *fixture)
{
	unsigned int i;
	selmon = mons;
	mons->mfact = .85f;
	mons->tagset[0] = 1;
	mons->nmaster = 5;
	for (i = 0; i < LENGTH(legacy_clients); i++) fixture->clients[i].cfact = 9.0f;
	/* Missing saved windows must not shift later clients' weights. */
	detach(&fixture->clients[1]);
	detachstack(&fixture->clients[1]);
	attach(&fixture->clients[3]);
	attachstack(&fixture->clients[3]);
}

static void
assert_rejected_unchanged(FILE *file, WeightFixture *fixture)
{
	WeightFixture before;
	Monitor *old_mons = mons, *old_selmon = selmon;
	unsigned int old_calls = live_calls, old_applies = forest_applies;
	BspWorkspace *old_views = checkpoint_forest.views;
	FILE *forest_before = forest_snapshot(&checkpoint_forest);
	memcpy(&before, fixture, sizeof before);
	assert(!restore_checkpoint(file));
	assert(memcmp(&before, fixture, sizeof before) == 0);
	assert(mons == old_mons && selmon == old_selmon);
	assert(live_calls == old_calls && forest_applies == old_applies);
	assert(checkpoint_forest.views == old_views);
	assert_forest_snapshot(forest_before, &checkpoint_forest);
	assert(fclose(forest_before) == 0);
}
