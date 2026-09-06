static void
test_mouse_coordinates(void)
{
	BspWorkspace *view = mouse_setup();
	Client *source = wintoclient(1);
	Client *next = source->next;
	BspResize expected;
	BspForest copy = {0};
	BspWorkspace *oracle;
	uint64_t ids[] = {1, 2};

	/* Compare the real event loop against an engine snapshot at release. */
	oracle = bsp_workspace(&copy, 0, 1, 1); assert(oracle);
	assert(bsp_tree_sync(&oracle->tree, ids, 2, 0, bsp_monitor_bounds(selmon)));
	bsp_tree_layout(&oracle->tree, bsp_monitor_bounds(selmon), 5);
	assert(bsp_resize_begin(&oracle->tree, 1, 500, 420, &expected));
	assert(bsp_resize_update(&oracle->tree, &expected, 623, 420));
	push_pointer(ButtonRelease, Button1, 900, 800, 1);
	push_pointer(MotionNotify, 0, 510, 420, 20);
	push_pointer(MotionNotify, 0, 610, 420, 21); /* throttled */
	push_pointer(ButtonRelease, Button3, 623, 420, 22);
	bsp_resize_mouse(source);
	assert(fabs(view->tree.root->ratio - oracle->tree.root->ratio) < 1e-9);
	assert(source->next == next && source->mon == selmon && !source->isfloating);
	assert(f.grabs == 1 && f.ungrabs == 1 && !f.grabbed && !f.moves && !f.raises);
	assert(f.consumed == 4 && f.enter_checks == 1);
	bsp_forest_clear(&copy);
	view = mouse_setup();
	push_pointer(ButtonRelease, Button3, 600, 420, 1);
	bsp_resize_mouse(wintoclient(1)); assert(view->tree.root->ratio == 0.6);
	/* The engine versions focus changes too: cancel rather than rebasing
	 * an in-flight snapshot onto the newly focused leaf. */
	view = mouse_setup();
	push_event(PropertyNotify)->xproperty.atom = ChangeFocus;
	push_pointer(ButtonRelease, Button3, 600, 420, 1);
	bsp_resize_mouse(wintoclient(1));
	assert(view->tree.root->ratio == 0.5 && f.consumed == 1 && f.ungrabs == 1);
}

static void
test_mouse_cancellation(void)
{
	int change;
	for (change = ChangeFloat; change <= ChangeMonitor; change++) {
		if (change == ChangeFocus) continue;
		mouse_setup();
		push_event(PropertyNotify)->xproperty.atom = (Atom)change;
		push_pointer(ButtonRelease, Button3, 800, 700, 1);
		bsp_resize_mouse(wintoclient(1));
		assert(f.consumed == 1 && f.ungrabs == 1 && !f.grabbed && !f.moves);
	}
	mouse_setup();
	push_event(DestroyNotify)->xdestroywindow.window = 1;
	bsp_resize_mouse(wintoclient(1));
	assert(!wintoclient(1) && f.ungrabs == 1 && !f.grabbed);
	mouse_setup(); push_event(UnmapNotify)->xunmap.window = 1;
	bsp_resize_mouse(wintoclient(1)); assert(!wintoclient(1) && f.ungrabs == 1);
	mouse_setup(); push_event(MapRequest);
	bsp_resize_mouse(wintoclient(1)); assert(wintoclient(99) && f.ungrabs == 1);
	mouse_setup(); push_event(ConfigureNotify);
	bsp_resize_mouse(wintoclient(1)); assert(f.ungrabs == 1 && selmon->wy == 50);
	mouse_setup(); f.grab_result = AlreadyGrabbed;
	bsp_resize_mouse(wintoclient(1)); assert(f.grabs == 1 && !f.ungrabs);
	mouse_setup(); f.pointer_ok = 0;
	bsp_resize_mouse(wintoclient(1)); assert(f.ungrabs == 1 && !f.consumed);
	mouse_setup(); remove_client(2);
	bsp_resize_mouse(wintoclient(1)); assert(!f.grabs);
	mouse_setup(); selmon->lt[0] = &tile_layout;
	bsp_resize_mouse(wintoclient(1)); assert(!f.grabs);
}
