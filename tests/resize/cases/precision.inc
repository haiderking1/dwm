/* Float cfact storage must not move an unrelated sibling at a rounding tie. */
static void
test_precision_siblings(void)
{
	const float weights[][4] = {
		{ 1000000.0f, 0.001f, 1000000.0f, 1.0f },
		{ 1.0f, 0.001f, 1.999f, 1.0f },
		{ 0.25f, 1.125f, 7.25f, 13.5f }
	};
	size_t k;
	unsigned int n, i;
	int delta;
	for (k = 0; k < sizeof(weights) / sizeof(weights[0]); ++k)
	for (n = 3; n <= 4; ++n) {
		TileResize state;
		int before_y[4], before_h[4], previous, wanted, moved = 0;
		fixture_init(n, 0);
		f.mon->wh = 897;
		for (i = 0; i < n; ++i) f.clients[i]->cfact = weights[k][i];
		tile(f.mon);
		f.mon->sel = f.clients[1];
		for (i = 0; i < n; ++i) {
			before_y[i] = f.clients[i]->y; before_h[i] = f.clients[i]->h;
		}
		assert(resize_begin(&state, f.mon->sel, 200,
		                    f.mon->sel->y + HEIGHT(f.mon->sel) - 1));
		assert(state.upper == 1 && state.lower == 2);
		previous = f.clients[1]->h;
		for (delta = -700; delta < 700; ++delta) {
			assert(resize_update(&state, 200, state.starty + delta));
			wanted = MAX(before_y[1] + state.minimum,
			             MIN(before_y[2] + before_h[2] + 4 - state.minimum, before_y[2] + delta));
			assert(abs(f.clients[2]->y - wanted) <= 1);
			assert(fabs((double)f.clients[1]->cfact + f.clients[2]->cfact
			            - ((double)weights[k][1] + weights[k][2]))
			       <= ((double)weights[k][1] + weights[k][2]) * 8 * FLT_EPSILON);
			if (f.clients[1]->h != previous) ++moved;
			previous = f.clients[1]->h;
			for (i = 0; i < n; ++i) {
				if (i == 1 || i == 2) continue;
				assert(f.clients[i]->cfact == weights[k][i]);
				assert(f.clients[i]->y == before_y[i] && f.clients[i]->h == before_h[i]);
			}
		}
		assert(moved > 100);
		free(state.slots);
		fixture_dispose();
	}
}
