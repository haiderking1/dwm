static void
test_preview_and_occupied_drop(void)
{
	fixture_init();
	push_pointer(MotionNotify, 0, 250, 70, 100, expect_original);
	push_dispatch(Expose, root, expect_preview);
	push_pointer(ButtonRelease, Button1, 250, 70, 110, expect_preview);
	drag_tiled(f.source);
	expect_cleanup(1);
	expect_order(1);
	assert(f.consumed == 3 && f.dispatches == 1);
	assert(f.nmoves == 5 && f.resizes == 2 && f.arrangements == 1);
	expect_move(0, SOURCE, 220, 40);
	expect_move(1, SOURCE, 220, 40);
	expect_move(2, SOURCE, 10, 40); /* Explicit restoration precedes the swap. */
	expect_move(3, TARGET, 10, 40);
	expect_move(4, SOURCE, 210, 40);
	fixture_dispose();
}

static void
test_empty_and_bar_snapback(void)
{
	const int points[][2] = { {350, 250}, {250, 10}, {650, 100} };
	size_t i;
	for (i = 0; i < sizeof(points) / sizeof(points[0]); ++i) {
		int x = points[i][0], y = points[i][1];
		fixture_init();
		push_pointer(MotionNotify, 0, x, y, 100, NULL);
		push_pointer(ButtonRelease, Button1, x, y, 101, expect_preview);
		drag_tiled(f.source);
		expect_cleanup(1);
		expect_order(0);
		assert(f.consumed == 2 && f.arrangements == 1 && f.resizes == 0);
		/* arrange saw identical logical rectangles and made no X request. */
		assert(f.nmoves == 2);
		expect_move(0, SOURCE, 10 + x - 40, 40 + y - 70);
		expect_move(1, SOURCE, 10, 40);
		fixture_dispose();
	}
}

static void
test_release_coordinates(void)
{
	int mode;
	for (mode = 0; mode < 3; ++mode) {
		fixture_init();
		if (mode == 1)
			push_pointer(MotionNotify, 0, 350, 250, 100, NULL);
		if (mode == 2)
			push_pointer(MotionNotify, 0, 250, 70, 100, NULL);
		/* No motion, empty-to-occupied, then occupied-to-empty. */
		push_pointer(ButtonRelease, Button1, mode == 2 ? 350 : 250,
		             mode == 2 ? 250 : 70, 101, mode ? expect_preview : expect_original);
		drag_tiled(f.source);
		expect_cleanup(1);
		expect_order(mode != 2);
		assert(f.consumed == (size_t)(mode ? 2 : 1));
		assert(f.raises == (mode ? 1 : 0));
		expect_move(mode ? 1 : 0, SOURCE, 10, 40);
		fixture_dispose();
	}
}

static void
test_short_click_and_threshold(void)
{
	int delta;
	for (delta = 0; delta <= 4; ++delta) {
		fixture_init();
		/* Start at the source's right border, beside the next tile. */
		f.startx = 193;
		f.monitors[0].step = 184;
		f.target->x = f.windows[1].x = 194;
		f.target_before = *f.target;
		push_pointer(ButtonRelease, Button1, 193 + delta, 70, 100, expect_original);
		drag_tiled(f.source);
		expect_cleanup(1);
		expect_order(delta == 4);
		assert(f.consumed == 1 && f.raises == 0);
		assert(f.nmoves == (size_t)(delta == 4 ? 3 : 1));
		fixture_dispose();
	}
}

static void
test_cross_monitor_drop(void)
{
	Monitor *left, *right;
	fixture_init();
	left = &f.monitors[0];
	right = &f.monitors[1];
	f.source->next = f.source->snext = NULL;
	right->clients = right->stack = right->sel = f.target;
	f.target->mon = right;
	f.target->tags = 2;
	f.target->x = f.windows[1].x = 410;
	f.source_before = *f.source;
	f.target_before = *f.target;
	push_pointer(MotionNotify, 0, 450, 70, 100, NULL);
	push_pointer(ButtonRelease, Button1, 450, 70, 101, expect_preview);
	drag_tiled(f.source);
	expect_cleanup(1);
	assert(f.consumed == 2 && f.updates == 1 && f.arrangements == 2);
	assert(f.arranged[0] == left && f.arranged[1] == right);
	assert(left->clients == f.target && left->stack == f.target && left->sel == f.target);
	assert(right->clients == f.source && right->stack == f.source && right->sel == f.source);
	assert(!f.source->next && !f.source->snext && !f.target->next && !f.target->snext);
	assert(f.source->mon == right && f.target->mon == left);
	assert(f.source->tags == 2 && f.target->tags == 1);
	assert(!f.source->isfloating && !f.target->isfloating);
	assert(f.source->x == 410 && f.target->x == 10);
	assert(f.source->y == 40 && f.target->y == 40);
	expect_server_matches(f.source);
	expect_server_matches(f.target);
	expect_move(1, SOURCE, 10, 40);
	fixture_dispose();
}
