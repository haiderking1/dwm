static void
test_drag(void)
{
	BspWorkspace *view = mouse_setup(), *dormant, *dest, *dest_dormant;
	Client *a = wintoclient(1), *b = wintoclient(2), *other;
	Monitor *origin = selmon, *destination;
	uint64_t aid = bsp_find(&view->tree, 1)->id, bid = bsp_find(&view->tree, 2)->id;
	int bx = b->x, by = b->y;
	uint64_t stale_source[] = {1, 2, 10}, stale_dest[] = {11, 10, 1};
	uint64_t dormant_a, dormant_b;

	/* Release invokes the list swap and then the actual tree adapter. */
	f.startx = a->x + 10; f.starty = a->y + 10;
	push_pointer(ButtonRelease, Button1, bx + 10, by + 10, 1);
	drag_tiled(a);
	assert(!f.grabbed && f.ungrabs == 1 && f.updates == 1);
	assert(bsp_find(&view->tree, 1)->id == bid && bsp_find(&view->tree, 2)->id == aid);
	assert(a->x == bx && a->y == by);
	assert(origin->clients == b && origin->sel == a);
	/* Both monitors own saved combined-tag views. The destination currently
	 * uses weighted tile, but all its BSP slots still need replacement. */
	origin->tagset[0] = 3; arrange(origin); dormant = bsp_active(origin);
	origin->tagset[0] = 1; arrange(origin);
	destination = add_monitor(1);
	other = add_client(destination, 10, 1);
	add_client(destination, 11, 1); destination->sel = other;
	arrange(destination); dest = bsp_active(destination);
	destination->tagset[0] = 3; arrange(destination); dest_dormant = bsp_active(destination);
	destination->tagset[0] = 1; arrange(destination);
	view->tree.root->ratio = 0.61; dest->tree.root->ratio = 0.73;
	arrange(origin); arrange(destination);
	aid = bsp_find(&view->tree, 1)->id; bid = bsp_find(&dest->tree, 10)->id;
	bx = other->x; by = other->y;
	/* A previous sendmon can leave an incoming identity in dormant views.
	 * Preparation must remove it before replace, not lose the occupied slot. */
	assert(bsp_tree_sync(&dormant->tree, stale_source, 3, 2, bsp_monitor_bounds(origin)));
	assert(bsp_tree_sync(&dest_dormant->tree, stale_dest, 3, 11, bsp_monitor_bounds(destination)));
	dormant_a = bsp_find(&dormant->tree, 1)->id;
	dormant_b = bsp_find(&dest_dormant->tree, 10)->id;
	destination->lt[0] = &tile_layout;
	f.startx = a->x + 10; f.starty = a->y + 10;
	push_pointer(ButtonRelease, Button1, bx + 10, by + 10, 2);
	drag_tiled(a);
	assert(a->mon == destination && other->mon == origin && f.updates == 2);
	assert(bsp_find(&view->tree, 10)->id == aid && !bsp_find(&view->tree, 1));
	assert(bsp_find(&dest->tree, 1)->id == bid && !bsp_find(&dest->tree, 10));
	assert(bsp_find(&dormant->tree, 10) && !bsp_find(&dormant->tree, 1));
	assert(bsp_find(&dest_dormant->tree, 1) && !bsp_find(&dest_dormant->tree, 10));
	assert(bsp_find(&dormant->tree, 10)->id == dormant_a);
	assert(bsp_find(&dest_dormant->tree, 1)->id == dormant_b);
	assert(view->tree.root->ratio == 0.61 && dest->tree.root->ratio == 0.73);
	destination->lt[0] = &bsp_layout; arrange(destination);
	assert(bsp_find(&dest->tree, 1)->id == bid && a->x == bx && a->y == by);
}
