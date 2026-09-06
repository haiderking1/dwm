static void
test_same_monitor_swaps(void)
{
	Fixture f, before;
	size_t count, a, b, selected;

	/* Every ordered pair covers adjacency, reversed arguments, head and tail. */
	for (count = 2; count <= 6; ++count)
		for (a = 0; a < count; ++a)
			for (b = 0; b < count; ++b) {
				if (a == b)
					continue;
				for (selected = 0; selected <= count; ++selected) {
					fixture_init(&f, count, 1);
					f.monitors[0].sel = selected < count ? &f.clients[selected] : NULL;
					memcpy(&before, &f, sizeof(f));
					expect_swap(&f, &f.clients[a], &f.clients[b]);
					/* A second swap must restore every field and both lists. */
					expect_swap(&f, &f.clients[b], &f.clients[a]);
					expect_unchanged(&f, &before);
				}
			}
}
