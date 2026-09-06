static void
test_failed_grab(void)
{
	const int failures[] = { AlreadyGrabbed, GrabInvalidTime, GrabNotViewable, GrabFrozen };
	size_t i;
	for (i = 0; i < sizeof(failures) / sizeof(failures[0]); ++i) {
		fixture_init();
		f.grab_result = failures[i];
		push_pointer(ButtonRelease, Button1, 250, 70, 100, NULL);
		drag_tiled(f.source);
		expect_original();
		expect_server_matches(f.source);
		expect_server_matches(f.target);
		assert(f.grabs == 1 && !f.grabbed && f.ungrabs == 0 && f.queries == 0);
		assert(f.consumed == 0 && f.nmoves == 0 && f.raises == 0);
		assert(f.syncs == 0 && f.enter_checks == 0 && f.publishes == 0 && f.flushes == 0);
		fixture_dispose();
	}
}

static void
test_failed_root_pointer(void)
{
	fixture_init();
	f.pointer_ok = 0;
	push_pointer(ButtonRelease, Button1, 250, 70, 100, NULL);
	drag_tiled(f.source);
	expect_original();
	expect_server_matches(f.source);
	expect_server_matches(f.target);
	assert(f.grabs == 1 && f.queries == 1 && f.ungrabs == 1 && !f.grabbed);
	assert(f.consumed == 0 && f.nmoves == 0 && f.raises == 0);
	assert(f.syncs == 0 && f.enter_checks == 0 && f.publishes == 0 && f.flushes == 0);
	fixture_dispose();
}

static void
test_source_destroyed(void)
{
	fixture_init();
	push_pointer(MotionNotify, 0, 250, 70, 100, NULL);
	push_dispatch(DestroyNotify, SOURCE, expect_preview);
	/* Must stop on destruction, rather than consuming this release. */
	push_pointer(ButtonRelease, Button1, 250, 70, 101, NULL);
	drag_tiled(f.source);
	expect_cleanup(0);
	assert(f.consumed == 2 && f.dispatches == 1 && f.destroyed == SOURCE);
	assert(f.source == NULL && wintoclient(SOURCE) == NULL && !f.windows[0].alive);
	assert(f.monitors[0].clients == f.target && f.monitors[0].stack == f.target);
	assert(f.monitors[0].sel == f.target && !f.target->next && !f.target->snext);
	assert(f.nmoves == 1 && f.raises == 1 && f.arrangements == 0 && f.updates == 0);
	assert(f.target->x == 210 && f.target->y == 40 && !f.target->isfloating);
	expect_server_matches(f.target);
	fixture_dispose();
}

static void
expect_target_gone(void)
{
	assert(f.target == NULL && wintoclient(TARGET) == NULL && !f.windows[1].alive);
	assert(f.monitors[0].clients == f.source && f.monitors[0].stack == f.source);
	assert(f.source->next == NULL && f.source->snext == NULL);
	assert(f.source->x == 10 && f.source->y == 40 && !f.source->isfloating);
	assert(f.grabbed && f.updates == 0);
}

static void
test_target_destroyed(void)
{
	fixture_init();
	push_pointer(MotionNotify, 0, 250, 70, 100, NULL);
	push_dispatch(DestroyNotify, TARGET, expect_preview);
	push_pointer(ButtonRelease, Button1, 250, 70, 101, expect_target_gone);
	drag_tiled(f.source);
	expect_cleanup(1);
	assert(f.consumed == 3 && f.dispatches == 1 && f.destroyed == TARGET);
	assert(f.target == NULL && wintoclient(TARGET) == NULL);
	assert(f.monitors[0].clients == f.source && f.monitors[0].stack == f.source);
	assert(f.monitors[0].sel == f.source && !f.source->next && !f.source->snext);
	assert(f.source->x == 10 && f.source->y == 40 && !f.source->isfloating);
	assert(f.updates == 0 && f.arrangements == 1 && f.resizes == 0 && f.nmoves == 3);
	expect_move(2, SOURCE, 10, 40);
	expect_server_matches(f.source);
	fixture_dispose();
}
