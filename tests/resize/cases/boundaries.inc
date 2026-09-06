static void
test_pairwise_boundaries(void)
{
	TileResize state;
	Client before[6];
	unsigned int i;
	int delta, boundary;
	fixture_init(6, 2);
	for (i = 0; i < 6; ++i) f.clients[i]->cfact = (float)i + 1;
	f.mon->sel = f.clients[3];
	tile(f.mon);
	for (i = 0; i < 6; ++i) before[i] = *f.clients[i];
	boundary = f.clients[3]->y;
	assert(resize_begin(&state, f.clients[3], 200, boundary + 1));
	assert(state.upper == 2 && state.lower == 3);
	for (delta = -100; delta <= 100; ++delta) {
		assert(resize_update(&state, 200, boundary + 1 + delta));
		assert(abs(f.clients[3]->y - boundary - delta) <= 1);
		assert(fabs((double)f.clients[2]->cfact + f.clients[3]->cfact - 7.0) < 0.000001);
		for (i = 0; i < 6; ++i) {
			assert(!f.clients[i]->isfloating && f.clients[i]->mon == f.mon);
			if (i == 2 || i == 3) continue;
			assert(f.clients[i]->cfact == before[i].cfact);
			assert(f.clients[i]->y == before[i].y && f.clients[i]->h == before[i].h);
		}
	}
	assert(resize_update(&state, 300, boundary + 51));
	assert(WIDTH(f.clients[0]) == 650 && WIDTH(f.clients[3]) == 350);
	assert(abs(f.clients[3]->y - boundary - 50) <= 1);
	assert(resize_update(&state, INT_MAX, INT_MIN));
	assert(f.mon->mfact == 0.95f);
	assert(f.clients[2]->cfact >= RESIZE_WEIGHT_MIN && HEIGHT(f.clients[2]) >= 32);
	assert(resize_update(&state, INT_MIN, INT_MAX));
	assert(f.mon->mfact == 0.05f && HEIGHT(f.clients[3]) >= 32);
	assert(resize_update(&state, 200, boundary + 1));
	for (i = 0; i < 6; ++i) {
		assert(f.clients[i]->cfact == before[i].cfact);
		assert(f.clients[i]->x == before[i].x && f.clients[i]->y == before[i].y);
		assert(f.clients[i]->w == before[i].w && f.clients[i]->h == before[i].h);
	}
	free(state.slots);
	fixture_dispose();
}

static void
test_boundary_choice(void)
{
	unsigned int selected;
	int edge;
	for (selected = 0; selected < 6; ++selected)
	for (edge = 0; edge < 2; ++edge) {
		TileResize state;
		Client *c;
		unsigned int start = selected < 3 ? 0 : 3;
		unsigned int expected = selected == start ? selected
		                      : selected == start + 2 ? selected - 1
		                      : edge ? selected : selected - 1;
		fixture_init(6, 3);
		c = f.mon->sel = f.clients[selected];
		assert(resize_begin(&state, c, c->x + 10, c->y + (edge ? HEIGHT(c) - 1 : 1)));
		assert(state.upper == expected && state.lower == expected + 1);
		assert(state.horizontal && state.vertical);
		free(state.slots);
		fixture_dispose();
	}
	/* With no master, vertical motion alone can resize the only column. */
	fixture_init(3, 0);
	{
		TileResize state;
		assert(resize_begin(&state, f.mon->sel, 0, 0));
		assert(!state.horizontal && state.vertical);
		assert(resize_update(&state, 9999, 50));
		assert(f.mon->mfact == 0.55f && WIDTH(f.mon->sel) == 1000);
		free(state.slots);
	}
	fixture_dispose();
}

static void
test_weighted_swap(void)
{
	int heights[4], positions[4];
	Client *order[4];
	unsigned int i;
	fixture_init(4, 0);
	for (i = 0; i < 4; ++i) {
		f.clients[i]->cfact = 0.25f + (float)i;
		f.clients[i]->snext = f.clients[i]->next;
	}
	f.mon->stack = f.mon->clients;
	tile(f.mon);
	for (i = 0; i < 4; ++i) {
		heights[i] = HEIGHT(f.clients[i]); positions[i] = f.clients[i]->y;
		order[i] = f.clients[i];
	}
	assert(drag_target(f.mon, f.clients[1], f.clients[3]->x + 1, f.clients[3]->y + 1) == f.clients[3]);
	assert(drag_swap(f.clients[1], f.clients[3]));
	order[1] = f.clients[3]; order[3] = f.clients[1];
	tile(f.mon);
	for (i = 0; i < 4; ++i) {
		assert(HEIGHT(order[i]) == heights[i] && order[i]->y == positions[i]);
		assert(order[i]->cfact == 0.25f + (float)i);
	}
	fixture_dispose();
}
