static void
expect_target_preview(void)
{
	expect_preview();
	assert(server_window(SOURCE)->x == 220 && server_window(SOURCE)->y == 40);
	assert(f.ungrabs == 0);
}

static void
test_other_buttons_do_not_release(void)
{
	fixture_init();
	push_pointer(MotionNotify, 0, 250, 70, 100, NULL);
	push_pointer(ButtonRelease, Button3, 350, 250, 101, expect_target_preview);
	push_pointer(ButtonPress, Button2, 20, 10, 102, expect_target_preview);
	push_pointer(ButtonRelease, Button2, 20, 10, 103, expect_target_preview);
	push_pointer(ButtonRelease, Button1, 250, 70, 104, expect_target_preview);
	drag_tiled(f.source);
	expect_cleanup(1);
	expect_order(1);
	assert(f.consumed == 5 && f.dispatches == 0 && f.raises == 2);
	assert(f.nmoves == 5);
	expect_move(0, SOURCE, 220, 40);
	expect_move(1, SOURCE, 220, 40);
	expect_move(2, SOURCE, 10, 40);
	fixture_dispose();
}

static void
expect_throttled(void)
{
	expect_original();
	assert(f.grabbed && f.nmoves == 0 && f.raises == 0 && f.flushes == 0);
	expect_server_matches(f.source);
}

static void
test_throttled_motion_release(void)
{
	fixture_init();
	push_pointer(MotionNotify, 0, 350, 250, 1, NULL);
	push_pointer(ButtonRelease, Button1, 250, 70, 2, expect_throttled);
	drag_tiled(f.source);
	expect_cleanup(1);
	expect_order(1);
	assert(f.consumed == 2 && f.raises == 0 && f.nmoves == 3);
	expect_move(0, SOURCE, 10, 40);
	fixture_dispose();
}
