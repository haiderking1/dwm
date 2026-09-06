static void
expect_mouse_cleanup(void)
{
	assert(f.grabs == 1 && f.ungrabs == 1 && !f.grabbed);
	assert(f.queries == 1 && f.syncs == 1 && f.enter_checks == 1);
	assert(f.publishes > 0);
}

static void
test_mouse_coordinates(void)
{
	int mode;
	for (mode = 0; mode < 4; ++mode) {
		Client *source;
		fixture_init(4, 1);
		source = f.mon->sel = f.clients[1];
		/* Tile geometry must ignore terminal increments and minimum hints. */
		source->incw = 37; source->inch = 29;
		source->minw = 2000; source->minh = 3000;
		if (mode) push_pointer(MotionNotify, 0, 225, 110, 100);
		if (mode == 2) push_pointer(MotionNotify, 0, 250, 120, 101);
		if (mode == 3) {
			push_event(Expose);
			push_event(ConfigureRequest);
		}
		push_pointer(ButtonRelease, Button3, 300, 150, 102);
		resize_tiled(source);
		expect_mouse_cleanup();
		assert(f.consumed == f.queued);
		assert(WIDTH(f.clients[0]) == 650 && WIDTH(source) == 350);
		assert(HEIGHT(source) == 350 && HEIGHT(f.clients[2]) == 250);
		assert(f.clients[3]->y == 620 && HEIGHT(f.clients[3]) == 300);
		assert(f.clients[3]->cfact == 1.0f);
		assert(f.mon->clients == f.clients[0] && f.mon->sel == source);
		assert(source->next == f.clients[2] && source->mon == f.mon && !source->isfloating);
		assert(f.arrangements == 0); /* geometry frames never restack */
		fixture_dispose();
	}
	/* Moving into another monitor's coordinates clamps this monitor's split.
	 * There is deliberately no sendmon, recttomon or XWarpPointer mock. */
	fixture_init(2, 1);
	push_pointer(ButtonRelease, Button3, -3000, 4000, 100);
	resize_tiled(f.mon->sel);
	expect_mouse_cleanup();
	assert(f.mon->mfact == 0.05f && !f.clients[0]->isfloating);
	assert(f.clients[0]->mon == f.mon && f.clients[0]->cfact == 1.0f);
	fixture_dispose();
	/* A click and a round trip restore the original non-pixel-aligned factor. */
	for (mode = 0; mode < 2; ++mode) {
		fixture_init(4, 1);
		f.mon->mfact = 0.55123f; tile(f.mon);
		if (mode) push_pointer(MotionNotify, 0, 300, 150, 100);
		push_pointer(ButtonRelease, Button3, f.startx, f.starty, 101);
		resize_tiled(f.mon->sel);
		assert(f.mon->mfact == 0.55123f);
		assert(f.clients[0]->cfact == 1.0f);
		fixture_dispose();
	}
}

static void
test_mouse_buttons(void)
{
	unsigned int button;
	fixture_init(4, 1);
	f.mon->sel = f.clients[1];
	for (button = Button1; button <= Button5; ++button) {
		push_pointer(ButtonPress, button, 999, 999, 100);
		if (button != Button3) push_pointer(ButtonRelease, button, 999, 999, 101);
	}
	push_pointer(ButtonRelease, Button3, 300, 150, 102);
	resize_tiled(f.mon->sel);
	expect_mouse_cleanup();
	assert(f.consumed == f.queued && f.arrangements == 0);
	assert(HEIGHT(f.clients[1]) == 350 && WIDTH(f.clients[0]) == 650);
	fixture_dispose();
}
