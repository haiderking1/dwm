static void
test_pending_configure_request(void)
{
	XEvent *event;
	fixture_init();
	push_pointer(MotionNotify, 0, 250, 70, 100, NULL);
	event = push_dispatch(ConfigureRequest, SOURCE, expect_preview);
	event->xconfigurerequest.value_mask = CWX | CWY | CWWidth | CWHeight;
	event->xconfigurerequest.x = 300;
	event->xconfigurerequest.y = 0;
	event->xconfigurerequest.width = 999;
	event->xconfigurerequest.height = 999;
	push_pointer(ButtonRelease, Button1, 250, 70, 101, expect_preview);
	drag_tiled(f.source);
	expect_cleanup(1);
	expect_order(1);
	assert(f.consumed == 3 && f.configure_replies == 1 && f.dispatches == 1);
	assert(f.nmoves == 5 && f.resizes == 2);
	fixture_dispose();
}

static void
expect_reconfigured(void)
{
	assert(f.grabbed && f.dispatches == 1 && f.arrangements == 1);
	assert(f.source->x == 30 && f.source->y == 100);
	assert(f.target->x == 230 && f.target->y == 100);
	assert(f.source->w == 160 && f.source->h == 130);
	assert(f.target->w == 160 && f.target->h == 140);
	assert(!f.source->isfloating && !f.target->isfloating);
	assert(f.monitors[0].clients == f.source && f.source->next == f.target);
	assert(f.updates == 0 && f.focuses == 0);
	expect_server_matches(f.target);
}

static void
test_pending_monitor_geometry(void)
{
	int occupied;
	for (occupied = 0; occupied <= 1; ++occupied) {
		XEvent *event;
		fixture_init();
		push_pointer(MotionNotify, 0, 250, 70, 100, NULL);
		event = push_dispatch(ConfigureNotify, root, expect_preview);
		event->xconfigure.x = 30;
		event->xconfigure.y = 100;
		event->xconfigure.width = 160;
		event->xconfigure.height = 120;
		/* Occupied only in the NEW target rectangle, below its old bottom. */
		push_pointer(ButtonRelease, Button1, occupied ? 250 : 350,
		             occupied ? 210 : 250, 101, expect_reconfigured);
		drag_tiled(f.source);
		expect_cleanup(1);
		expect_order(occupied);
		assert(f.consumed == 3 && f.dispatches == 1 && f.arrangements == 2);
		assert(f.resizes == (occupied ? 4 : 2));
		assert(f.nmoves == (size_t)(occupied ? 7 : 5));
		/* Restore CURRENT logical coordinates, not the pre-configure origin. */
		expect_move(4, SOURCE, 30, 100);
		fixture_dispose();
	}
}

static void
expect_dock_applied(void)
{
	assert(f.dock_events == 1 && f.dispatches == 0 && f.grabbed);
	assert(f.source->y == 80 && f.target->y == 80);
	assert(f.source->x == 10 && f.target->x == 210);
	assert(f.monitors[0].clients == f.source && f.source->next == f.target);
	assert(!f.source->isfloating && f.updates == 0);
}

static void
test_pending_dock_workarea(void)
{
	XEvent *event;
	fixture_init();
	push_pointer(MotionNotify, 0, 250, 70, 100, NULL);
	event = push_dispatch(PropertyNotify, root, expect_preview);
	event->xproperty.atom = DOCK_CHANGE;
	push_pointer(ButtonRelease, Button1, 250, 200, 101, expect_dock_applied);
	drag_tiled(f.source);
	expect_cleanup(1);
	expect_order(1);
	assert(f.consumed == 3 && f.dock_events == 1 && f.dispatches == 0);
	assert(f.arrangements == 2 && f.resizes == 4 && f.nmoves == 7);
	expect_move(4, SOURCE, 10, 80);
	fixture_dispose();
}

static void
test_layout_change_cancels(void)
{
	XEvent *event;
	fixture_init();
	push_pointer(MotionNotify, 0, 250, 70, 100, NULL);
	event = push_dispatch(PropertyNotify, root, expect_preview);
	event->xproperty.atom = FLOAT_LAYOUT;
	push_pointer(ButtonRelease, Button1, 250, 70, 101, NULL);
	drag_tiled(f.source);
	expect_cleanup(1);
	expect_order(0);
	assert(f.consumed == 2 && f.dispatches == 1 && f.monitors[0].sellt == 1);
	assert(f.arrangements == 2 && f.resizes == 0 && f.nmoves == 2);
	expect_move(1, SOURCE, 10, 40);
	fixture_dispose();
}

static void
test_target_becomes_ineligible(void)
{
	XEvent *event;
	fixture_init();
	push_pointer(MotionNotify, 0, 250, 70, 100, NULL);
	event = push_dispatch(PropertyNotify, TARGET, expect_preview);
	event->xproperty.atom = FULLSCREEN_CHANGE;
	push_pointer(ButtonRelease, Button1, 250, 70, 101, NULL);
	drag_tiled(f.source);
	expect_cleanup(1);
	assert(f.consumed == 3 && f.dispatches == 1 && f.updates == 0);
	assert(f.monitors[0].clients == f.source && f.source->next == f.target && !f.target->next);
	assert(f.monitors[0].stack == f.source && f.source->snext == f.target && !f.target->snext);
	assert(f.source->x == 10 && f.source->y == 40 && !f.source->isfullscreen);
	assert(f.target->x == 210 && f.target->y == 40 && f.target->isfullscreen);
	assert(!f.source->isfloating && !f.target->isfloating);
	assert(f.resizes == 0 && f.nmoves == 3);
	expect_move(2, SOURCE, 10, 40);
	expect_server_matches(f.source);
	expect_server_matches(f.target);
	fixture_dispose();
}
