/* Model a successful operation by replacing slots in captured pointer arrays. */
static void
expect_swap(Fixture *f, Client *a, Client *b)
{
	Fixture expected;
	Client *orders[2][2][CAPACITY];
	size_t counts[2][2];
	size_t m, kind, i;
	const int cross = a->mon != b->mon;

	memcpy(&expected, f, sizeof(expected));
	for (m = 0; m < 2; ++m) {
		Monitor *em = &expected.monitors[m];
		for (kind = 0; kind < 2; ++kind) {
			Client *head = kind ? f->monitors[m].stack : f->monitors[m].clients;
			counts[m][kind] = capture_list(head, orders[m][kind], (int)kind);
			for (i = 0; i < counts[m][kind]; ++i) {
				Client **slot = &orders[m][kind][i];
				if (!kind || cross) {
					if (*slot == a)
						*slot = b;
					else if (*slot == b)
						*slot = a;
				}
			}
			for (i = 0; i < counts[m][kind]; ++i) {
				Client *ec = &expected.clients[orders[m][kind][i] - f->clients];
				Client *next = i + 1 < counts[m][kind] ? orders[m][kind][i + 1] : NULL;
				if (kind)
					ec->snext = next;
				else
					ec->next = next;
			}
			head = counts[m][kind] ? orders[m][kind][0] : NULL;
			if (kind)
				em->stack = head;
			else
				em->clients = head;
		}
		if (cross) {
			if (em->sel == a)
				em->sel = b;
			else if (em->sel == b)
				em->sel = a;
		}
	}
	if (cross) {
		expected.clients[a - f->clients].mon = b->mon;
		expected.clients[b - f->clients].mon = a->mon;
		expected.clients[a - f->clients].tags = b->tags;
		expected.clients[b - f->clients].tags = a->tags;
	}
	expected.clients[a - f->clients].cfact = b->cfact;
	expected.clients[b - f->clients].cfact = a->cfact;
	assert(a->cfact != b->cfact);
	assert(drag_swap(a, b) != 0);
	/* Includes unchanged geometry, tags, selection and nonparticipant fields. */
	expect_unchanged(f, &expected);
	for (m = 0; m < 2; ++m) {
		expect_list(f->monitors[m].clients, orders[m][0], counts[m][0], 0);
		expect_list(f->monitors[m].stack, orders[m][1], counts[m][1], 1);
	}
}

static void
rotate_stack(Fixture *f, size_t monitor, size_t offset, size_t count, size_t shift)
{
	size_t i;
	f->monitors[monitor].stack = &f->clients[offset + shift];
	for (i = 0; i < count; ++i) {
		Client *c = &f->clients[offset + (shift + i) % count];
		c->snext = i + 1 < count
		    ? &f->clients[offset + (shift + i + 1) % count] : NULL;
	}
}
