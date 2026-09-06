static void
test_routing(void)
{
	BspWorkspace *view = mouse_setup();
	Arg delta = {.f = 0.05f};
	uint64_t first;

	routed_weighted = routed_pop = 0;
	push_pointer(ButtonRelease, Button3, 600, 420, 1);
	resizemouse(NULL);
	assert(view->tree.root->ratio == 0.6 && !routed_weighted);
	setmfact(&delta); assert(view->tree.root->ratio > 0.64);
	first = view->tree.root->child[0]->window;
	zoom(NULL); assert(view->tree.root->child[0]->window != first && !routed_pop);
	selmon->lt[0] = &tile_layout;
	resizemouse(NULL); assert(routed_weighted == 1);
	selmon->lt[0] = &monocle_layout;
	resizemouse(NULL); assert(routed_weighted == 1 && f.grabs == 1);
	wintoclient(1)->isfullscreen = 1;
	selmon->lt[0] = &bsp_layout;
	resizemouse(NULL); assert(routed_weighted == 1 && f.grabs == 1);
}
