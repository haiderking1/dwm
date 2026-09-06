static void
test_layout(void)
{
	Client *a, *b, *c;
	BspWorkspace *view, *combined;
	BspNode *leaf;
	uint64_t id;
	int x, y, w, h, before;

	reset();
	/* add_client prepends, exactly as dwm manage does. */
	c = add_client(selmon, 3, 1);
	b = add_client(selmon, 2, 1);
	a = add_client(selmon, 1, 1);
	assert(!bsp_forest.views);
	focus(b); assert(!bsp_forest.views);
	arrange(selmon); view = bsp_active(selmon);
	assert(view->tree.root->child[0]->window == a->win);
	assert(bsp_find(&view->tree, b->win)->parent == bsp_find(&view->tree, c->win)->parent);
	assert(view->tree.minimum == 32);
	before = f.resizes;
	arrange(selmon);
	assert(f.resizes == before);
	assert(a->x == 0 && WIDTH(a) == 500 && a->y == 20 && HEIGHT(a) == 800);
	assert(b->x == 500 && b->y == 20 && HEIGHT(b) == 400);
	/* New mapping focus must not displace the cached insertion anchor. */
	focus(a); leaf = bsp_find(&view->tree, a->win); id = leaf->id;
	c = add_client(selmon, 4, 1); focus(c);
	assert(view->tree.focus == a->win);
	arrange(selmon);
	assert(bsp_find(&view->tree, a->win)->id == id);
	assert(bsp_find(&view->tree, a->win)->parent == bsp_find(&view->tree, c->win)->parent);
	assert(view->tree.focus == c->win);
	view->tree.root->ratio = 0.63;
	selmon->tagset[0] = 3; arrange(selmon); combined = bsp_active(selmon);
	assert(combined != view && combined->tags == 3);
	selmon->tagset[0] = 1; arrange(selmon);
	assert(bsp_active(selmon) == view && view->tree.root->ratio == 0.63);
	/* A tiled fullscreen client retains identity and never receives resize. */
	x = a->x; y = a->y; w = a->w; h = a->h;
	a->isfullscreen = a->isfloating = 1; a->oldstate = 0;
	arrange(selmon);
	assert(bsp_find(&view->tree, a->win)->id == id);
	assert(a->x == x && a->y == y && a->w == w && a->h == h);
	a->oldstate = 1; arrange(selmon); assert(!bsp_find(&view->tree, a->win));
	b->tags = 2; c->isfloating = 1; arrange(selmon);
	assert(!bsp_find(&view->tree, b->win) && !bsp_find(&view->tree, c->win));
	remove_client(3);
	assert(!bsp_find(&view->tree, 3) && !bsp_find(&combined->tree, 3));
	bsp_forget_monitor(&bsp_forest, selmon->num); assert(!bsp_forest.views);
}

static void
test_actions(void)
{
	BspWorkspace *view = mouse_setup();
	BspNode *rootnode = view->tree.root;
	BspAxis axis = rootnode->axis;
	double ratio = rootnode->ratio;
	uint64_t first = rootnode->child[0]->window;

	bsp_adjust_selected(0.05); assert(rootnode->ratio > ratio);
	bsp_rotate_selected(NULL); assert(rootnode->axis != axis);
	bsp_zoom_selected(); assert(rootnode->child[0]->window != first);
	assert(view->tree.focus == 1 && selmon->clients->win == 1);
}
