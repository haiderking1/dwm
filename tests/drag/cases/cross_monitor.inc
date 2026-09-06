static void
cross_selections(size_t left, size_t right, size_t a, size_t b,
    size_t left_stack, size_t right_stack)
{
	Fixture f, before;
	size_t ls, rs;
	int reverse;

	/* Each selection can be NULL, the exchanged client, or any other client. */
	for (ls = 0; ls <= left; ++ls)
		for (rs = 0; rs <= right; ++rs)
			for (reverse = 0; reverse < 2; ++reverse) {
				fixture_init(&f, left, right);
				rotate_stack(&f, 0, 0, left, left_stack);
				rotate_stack(&f, 1, left, right, right_stack);
				f.monitors[0].sel = ls < left ? &f.clients[ls] : NULL;
				f.monitors[1].sel = rs < right ? &f.clients[left + rs] : NULL;
				memcpy(&before, &f, sizeof(f));
				if (reverse)
					expect_swap(&f, &f.clients[left + b], &f.clients[a]);
				else
					expect_swap(&f, &f.clients[a], &f.clients[left + b]);
				expect_swap(&f, &f.clients[a], &f.clients[left + b]);
				expect_unchanged(&f, &before);
			}
}

static void
test_cross_monitor_swaps(void)
{
	size_t left, right, a, b, ls, rs;

	/* Client slots and stack slots vary independently, including singleton lists. */
	for (left = 1; left <= 3; ++left)
		for (right = 1; right <= 3; ++right)
			for (a = 0; a < left; ++a)
				for (b = 0; b < right; ++b)
					for (ls = 0; ls < left; ++ls)
						for (rs = 0; rs < right; ++rs)
							cross_selections(left, right, a, b, ls, rs);
}
