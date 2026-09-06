static void
test_mouse_failures(void)
{
	int mode;
	for (mode = 0; mode < 7; ++mode) {
		fixture_init(mode == 0 ? 1 : 3, 0);
		if (mode == 1) f.mon->lt[0] = &monocle_layout;
		if (mode == 2) f.mon->sellt = 1;
		if (mode == 3) f.mon->sel->isfloating = 1;
		if (mode == 4) f.mon->sel->isfullscreen = 1;
		if (mode == 5) f.grab_result = AlreadyGrabbed;
		if (mode == 6) f.pointer_ok = 0;
		resize_tiled(f.mon->sel);
		assert(!f.grabbed && !f.consumed && !f.arrangements);
		assert(f.grabs == (mode >= 5));
		assert(f.ungrabs == (mode == 6));
		fixture_dispose();
	}
	fixture_init(3, 0);
	f.mon->wh = 96; tile(f.mon);
	resize_tiled(f.mon->sel); /* No height remains beyond the minima. */
	assert(f.grabs == 1 && f.ungrabs == 1 && !f.consumed);
	fixture_dispose();
	fixture_init(2, 1);
	f.mon->ww = 64; tile(f.mon);
	resize_tiled(f.mon->sel); /* Both columns already have their minimum. */
	assert(f.grabs == 1 && f.ungrabs == 1 && !f.consumed);
	fixture_dispose();
}

static void
test_mouse_lifecycle(void)
{
	int change, kind;
	for (change = ChangeMonocle; change < ChangeCount; ++change) {
		XEvent *ev;
		fixture_init(4, 1);
		push_pointer(MotionNotify, 0, 250, 125, 100);
		ev = push_event(PropertyNotify); ev->xproperty.atom = (Atom)change;
		push_pointer(ButtonRelease, Button3, 350, 200, 101);
		resize_tiled(f.mon->sel);
		expect_mouse_cleanup();
		assert(f.consumed == 2 && f.queued == 3);
		assert(f.dispatches == (change != ChangeDock));
		assert(f.docks == (change == ChangeDock));
		assert(f.mon->mfact == (change == ChangeFactor ? 0.7f : 0.6f));
		fixture_dispose();
	}
	for (kind = 0; kind < 8; ++kind) {
		XEvent *ev;
		fixture_init(4, 1);
		f.mon->sel = f.clients[1]; /* Source has a lower neighbor in stack. */
		push_pointer(MotionNotify, 0, 250, 125, 100);
		if (kind < 6) {
			Window victim = f.clients[kind % 3 + 1]->win;
			ev = push_event(kind < 3 ? DestroyNotify : UnmapNotify);
			if (kind < 3) ev->xdestroywindow.window = victim;
			else ev->xunmap.window = victim;
		} else {
			push_event(kind == 6 ? MapRequest : ConfigureNotify);
		}
		push_pointer(ButtonRelease, Button3, 350, 200, 101);
		resize_tiled(f.mon->sel);
		expect_mouse_cleanup();
		assert(f.consumed == 2 && f.dispatches == 1);
		assert(f.mon->mfact == 0.6f);
		fixture_dispose();
	}
	/* Even if changed geometry is first noticed at release, reject it before
	 * looking up an old neighbor or applying an old factor/weight baseline. */
	fixture_init(4, 1);
	{
		TileResize state;
		assert(resize_begin(&state, f.mon->sel, 200, 100));
		remove_client(f.clients[2]->win);
		assert(!resize_update(&state, 500, 500));
		assert(f.mon->mfact == 0.55f);
		free(state.slots);
	}
	fixture_dispose();
}
