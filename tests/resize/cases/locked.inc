static void
test_locked_boundaries(void)
{
	const float limits[] = { RESIZE_WEIGHT_MIN, RESIZE_WEIGHT_MAX };
	size_t k;
	for (k = 0; k < sizeof(limits) / sizeof(limits[0]); ++k) {
		TileResize state;
		fixture_init(3, 0);
		f.mon->sel = f.clients[1];
		f.clients[1]->cfact = f.clients[2]->cfact = limits[k];
		tile(f.mon);
		/* The nearer lower pair is locked at the weight limit. Use above. */
		assert(resize_begin(&state, f.mon->sel, 100,
		                    f.mon->sel->y + HEIGHT(f.mon->sel) - 1));
		assert(state.vertical && state.upper == 0 && state.lower == 1);
		free(state.slots);
		f.clients[0]->cfact = limits[k]; tile(f.mon);
		assert(!resize_begin(&state, f.mon->sel, 100, f.mon->sel->y));
		assert(!state.slots);
		fixture_dispose();
	}
}
