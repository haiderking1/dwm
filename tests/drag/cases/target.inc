static void
test_target_geometry(void)
{
	Fixture f, before;
	Client *c;
	int x, y, border;

	for (border = 0; border <= 3; ++border) {
		fixture_init(&f, 1, 0);
		c = &f.clients[0];
		c->x = -17;
		c->y = -11;
		c->w = 13;
		c->h = 7;
		c->bw = border;
		memcpy(&before, &f, sizeof(f));
		/* All interior points, all four edges, corners and border pixels. */
		for (x = -19; x <= 4; ++x)
			for (y = -13; y <= 5; ++y) {
				Client *expected = x >= -17 && x < -4 + 2 * border
				    && y >= -11 && y < -4 + 2 * border ? c : NULL;
				assert(drag_target(&f.monitors[0], NULL, x, y) == expected);
			}
		expect_unchanged(&f, &before);
	}
}

static void
test_target_stack(void)
{
	Fixture f, before;
	Monitor *m;
	Client *bottom, *top;
	int blocker;

	fixture_init(&f, 2, 1);
	m = &f.monitors[0];
	bottom = &f.clients[0];
	top = &f.clients[1];
	top->x = bottom->x;
	assert(m->clients == bottom && m->stack == top);
	memcpy(&before, &f, sizeof(f));
	assert(drag_target(m, NULL, 5, -15) == top);
	assert(drag_target(m, top, 5, -15) == bottom);
	assert(drag_target(m, bottom, 5, -15) == top);
	assert(drag_target(m, &f.clients[2], 5, -15) == top);
	assert(drag_target(&f.monitors[1], top, 5, -15) == &f.clients[2]);
	assert(drag_target(m, NULL, 1000, 1000) == NULL);
	expect_unchanged(&f, &before);

	for (blocker = 0; blocker < 3; ++blocker) {
		top->isfloating = blocker != 1;
		top->isfullscreen = blocker != 0;
		memcpy(&before, &f, sizeof(f));
		assert(drag_target(m, NULL, 5, -15) == NULL);
		assert(drag_target(m, top, 5, -15) == bottom);
		expect_unchanged(&f, &before);
		top->tags = 0;
		assert(drag_target(m, NULL, 5, -15) == bottom);
		top->tags = before.clients[1].tags;
		top->x = 100;
		assert(drag_target(m, NULL, 5, -15) == bottom);
		top->x = bottom->x;
	}
	top->isfloating = top->isfullscreen = 0;
	top->tags = 0;
	assert(drag_target(m, NULL, 5, -15) == bottom);
	bottom->tags = 0;
	assert(drag_target(m, NULL, 5, -15) == NULL);

	fixture_init(&f, 1, 0);
	assert(drag_target(&f.monitors[1], NULL, 5, -15) == NULL);
	assert(drag_target(m, &f.clients[0], 5, -15) == NULL);
	m->sellt = 0;
	memcpy(&before, &f, sizeof(f));
	assert(drag_target(m, NULL, 5, -15) == NULL);
	expect_unchanged(&f, &before);
	m->sellt = 1;
	assert(drag_target(m, NULL, 5, -15) == &f.clients[0]);
	m->seltags = 0;
	assert(drag_target(m, NULL, 5, -15) == NULL);
}
