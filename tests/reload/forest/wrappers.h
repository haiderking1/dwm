#ifndef RELOAD_FOREST_WRAPPERS_H
#define RELOAD_FOREST_WRAPPERS_H
#include "../../../wm/bsp/bsp.h"

static BspForest checkpoint_forest;
static unsigned int forest_applies;
static void (*checkpoint_before_apply)(const BspForest *);
static void (*checkpoint_before_arrange)(void);
#include "clean.h"

static int
bsp_checkpoint_write(FILE *file)
{
	return checkpoint_clean_forest(&checkpoint_forest) &&
	       bsp_forest_write(file, &checkpoint_forest);
}

static int
bsp_checkpoint_read(FILE *file, BspForest *out)
{
	BspForest parsed = {0};
	BspWorkspace *view, *other;
	if (!bsp_forest_read(file, &parsed)) return 0;
	for (view = parsed.views; view; view = view->next) {
		if (view->monitor < 0 || !view->tags || (view->tags & ~TAGMASK))
			goto invalid;
		for (other = view->next; other; other = other->next)
			if (other->monitor == view->monitor && other->tags == view->tags)
				goto invalid;
	}
	bsp_forest_clear(out);
	*out = parsed;
	return 1;
invalid:
	bsp_forest_clear(&parsed);
	return 0;
}

static void
bsp_checkpoint_apply(BspForest *parsed)
{
	if (checkpoint_before_apply) checkpoint_before_apply(parsed);
	live_calls++;
	forest_applies++;
	bsp_forest_clear(&checkpoint_forest);
	checkpoint_forest = *parsed;
	parsed->views = NULL;
	if (!checkpoint_clean_forest(&checkpoint_forest))
		bsp_forest_clear(&checkpoint_forest);
}
#endif
