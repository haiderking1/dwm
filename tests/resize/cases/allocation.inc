static void
test_allocation(void)
{
	unsigned int n, masters, i, start, count;
	int h, top, width, left;
	const int heights[] = { 1, 17, 60, 137, 900, 1081 };
	size_t k;
	for (n = 1; n <= 12; ++n)
	for (masters = 0; masters <= n + 1; ++masters)
	for (k = 0; k < sizeof(heights) / sizeof(heights[0]); ++k) {
		fixture_init(n, (int)masters);
		f.mon->wh = h = heights[k];
		for (i = 0; i < n; ++i) f.clients[i]->cfact = 0.25f + (float)i;
		tile(f.mon);
		for (start = 0; start < n; start += count) {
			unsigned int nm = MIN(n, masters);
			count = start == 0 && nm ? nm : n - start;
			top = f.mon->wy;
			left = start ? f.mon->wx + resize_split(f.mon->ww, f.mon->mfact) : f.mon->wx;
			width = nm == 0 || nm == n ? f.mon->ww
			      : start ? f.mon->ww - resize_split(f.mon->ww, f.mon->mfact)
			              : resize_split(f.mon->ww, f.mon->mfact);
			for (i = start; i < start + count; ++i) {
				Client *c = f.clients[i];
				assert(c->x == left && WIDTH(c) == width);
				assert(c->w > 0 && c->h > 0);
				if (h >= 5) assert(c->y + HEIGHT(c) <= f.mon->wy + h);
				if (h >= 5 * (int)count) {
					assert(c->y == top);
					assert(HEIGHT(c) >= MIN(32, h / (int)count));
					top += HEIGHT(c);
				}
			}
			if (h >= 5 * (int)count) assert(top == f.mon->wy + h);
		}
		fixture_dispose();
	}
	/* Filtering and nmaster=0 still partition one full-width column. */
	fixture_init(5, 0);
	f.clients[0]->tags = 2; f.clients[2]->isfloating = 1;
	tile(f.mon);
	assert(f.clients[1]->x == f.mon->wx && WIDTH(f.clients[1]) == 1000);
	assert(f.clients[1]->y == f.mon->wy);
	assert(f.clients[3]->y == f.clients[1]->y + HEIGHT(f.clients[1]));
	assert(f.clients[4]->y + HEIGHT(f.clients[4]) == f.mon->wy + f.mon->wh);
	fixture_dispose();
	/* Rounding assigns every remaining pixel, even below the preferred min. */
	for (h = 1; h < 300; ++h) {
		int minimum = resize_minimum(h, 7, 2), previous = 0;
		for (i = 1; i <= 7; ++i) {
			int edge = resize_edge(i, 7, h, minimum, (double)i, 7.0);
			assert(edge >= previous && edge <= h);
			previous = edge;
		}
		assert(previous == h);
	}
}

static void
test_weight_bounds(void)
{
	const float weights[] = { RESIZE_WEIGHT_MIN, 0.25f, 1.0f, 13.75f, RESIZE_WEIGHT_MAX };
	const double deltas[] = { -1e20, -1000, -1, 0, 1, 1000, 1e20 };
	size_t i, j, k;
	float a, b;
	assert(resize_weight(NAN) == RESIZE_WEIGHT_MIN);
	assert(resize_weight(-INFINITY) == RESIZE_WEIGHT_MIN);
	assert(resize_weight(INFINITY) == RESIZE_WEIGHT_MAX);
	for (i = 0; i < sizeof(weights) / sizeof(weights[0]); ++i)
	for (j = 0; j < sizeof(weights) / sizeof(weights[0]); ++j)
	for (k = 0; k < sizeof(deltas) / sizeof(deltas[0]); ++k) {
		double sum = (double)weights[i] + weights[j];
		resize_pair(weights[i], weights[j], deltas[k], &a, &b);
		assert(a >= RESIZE_WEIGHT_MIN && a <= RESIZE_WEIGHT_MAX);
		assert(b >= RESIZE_WEIGHT_MIN && b <= RESIZE_WEIGHT_MAX);
		assert(fabs((double)a + b - sum) <= sum * 0.0000001);
		if (!deltas[k]) assert(a == weights[i] && b == weights[j]);
	}
	fixture_init(3, 0);
	f.clients[0]->cfact = NAN; f.clients[1]->cfact = INFINITY;
	f.clients[2]->cfact = -1;
	tile(f.mon);
	assert(f.clients[0]->cfact == RESIZE_WEIGHT_MIN);
	assert(f.clients[1]->cfact == RESIZE_WEIGHT_MAX);
	assert(f.clients[2]->cfact == RESIZE_WEIGHT_MIN);
	fixture_dispose();
}
