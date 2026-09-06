static void
expect_rejected(Fixture *f, Client *a, Client *b)
{
	Fixture before;
	Client *orders[2][2][CAPACITY];
	size_t counts[2][2], m;

	memcpy(&before, f, sizeof(before));
	for (m = 0; m < 2; ++m) {
		counts[m][0] = capture_list(f->monitors[m].clients, orders[m][0], 0);
		counts[m][1] = capture_list(f->monitors[m].stack, orders[m][1], 1);
	}
	assert(drag_swap(a, b) == 0);
	expect_unchanged(f, &before);
	assert(drag_swap(b, a) == 0);
	expect_unchanged(f, &before);
	for (m = 0; m < 2; ++m) {
		expect_list(f->monitors[m].clients, orders[m][0], counts[m][0], 0);
		expect_list(f->monitors[m].stack, orders[m][1], counts[m][1], 1);
	}
}

static void
remove_member(Client *c, int stack)
{
	Client **slot = stack ? &c->mon->stack : &c->mon->clients;
	while (*slot && *slot != c)
		slot = stack ? &(*slot)->snext : &(*slot)->next;
	assert(*slot == c);
	*slot = stack ? c->snext : c->next;
}

static void
test_invalid_swaps(void)
{
	Fixture f;
	Client *a, *b, *bad;
	int cross, side, reason;

	fixture_init(&f, 3, 3);
	expect_rejected(&f, NULL, NULL);
	expect_rejected(&f, &f.clients[0], NULL);
	expect_rejected(&f, &f.clients[0], &f.clients[0]);
	fixture_init(&f, 1, 0);
	expect_rejected(&f, &f.clients[0], &f.clients[0]);

	for (cross = 0; cross < 2; ++cross)
		for (side = 0; side < 2; ++side)
			for (reason = 0; reason < 6; ++reason) {
				fixture_init(&f, 3, 3);
				a = &f.clients[0];
				b = &f.clients[cross ? 3 : 2];
				bad = side ? b : a;
				switch (reason) {
				case 0: bad->tags = 0; break;
				case 1: bad->tags = 1u << 24; break;
				case 2: bad->isfloating = 1; break;
				case 3: bad->isfullscreen = 1; break;
				case 4: bad->mon->sellt = 0; break;
				case 5: bad->mon->seltags = 0; break;
				}
				expect_rejected(&f, a, b);
			}
}

static void
test_missing_membership(void)
{
	Fixture f;
	Client *a, *b, *bad;
	size_t ai, bi;
	int cross, side, kind, empty;

	for (cross = 0; cross < 2; ++cross)
		for (ai = 0; ai < 3; ++ai)
			for (bi = 0; bi < 3; ++bi) {
				if (!cross && ai == bi)
					continue;
				for (side = 0; side < 2; ++side)
					for (kind = 0; kind < 2; ++kind)
						for (empty = 0; empty < 2; ++empty) {
							fixture_init(&f, 3, 3);
							a = &f.clients[ai];
							b = &f.clients[(cross ? 3u : 0u) + bi];
							bad = side ? b : a;
							if (!empty)
								remove_member(bad, kind);
							else if (kind)
								bad->mon->stack = NULL;
							else
								bad->mon->clients = NULL;
							expect_rejected(&f, a, b);
						}
				/* Both nodes missing, from different lists or the same list. */
				for (side = 0; side < 2; ++side)
					for (kind = 0; kind < 2; ++kind) {
						fixture_init(&f, 3, 3);
						a = &f.clients[ai];
						b = &f.clients[(cross ? 3u : 0u) + bi];
						remove_member(a, side);
						remove_member(b, kind);
						expect_rejected(&f, a, b);
					}
			}
}
