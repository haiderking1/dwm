static void
expect_move(size_t index, Window window, int x, int y)
{
	assert(index < f.nmoves);
	assert(f.moves[index].win == window);
	assert(f.moves[index].x == x && f.moves[index].y == y);
}

static void
expect_original(void)
{
	assert(f.source && f.target);
	assert(memcmp(f.source, &f.source_before, sizeof(*f.source)) == 0);
	assert(memcmp(f.target, &f.target_before, sizeof(*f.target)) == 0);
	assert(f.monitors[0].clients == f.source);
	assert(f.monitors[0].stack == f.source && f.monitors[0].sel == f.source);
	assert(f.updates == 0 && f.focuses == 0 && f.arrangements == 0);
}

static void
expect_preview(void)
{
	ServerWindow *w = server_window(SOURCE);
	expect_original();
	assert(f.grabbed && f.nmoves > 0 && f.raises > 0 && f.flushes > 0);
	assert(w->x != f.source->x || w->y != f.source->y);
	expect_move(f.nmoves - 1, SOURCE, w->x, w->y);
	assert(server_window(TARGET)->x == f.target->x);
	assert(server_window(TARGET)->y == f.target->y);
}

static void
expect_server_matches(Client *c)
{
	ServerWindow *w = server_window(c->win);
	assert(w->x == c->x && w->y == c->y);
}

static void
expect_cleanup(int has_source)
{
	assert(f.grabs == 1 && f.queries == 1 && f.ungrabs == 1 && !f.grabbed);
	assert(f.syncs == 1 && f.enter_pending == 0 && f.enter_checks == 3);
	assert(f.publishes > 0);
	assert(f.focuses == has_source);
	assert(f.focused == (has_source ? SOURCE : None));
}

static void
expect_order(int swapped)
{
	Client *first = swapped ? f.target : f.source;
	Client *second = swapped ? f.source : f.target;
	Monitor *m = &f.monitors[0];
	assert(m->clients == first && first->next == second && second->next == NULL);
	assert(m->stack == f.source && f.source->snext == f.target && f.target->snext == NULL);
	assert(m->sel == f.source);
	assert(f.monitors[1].clients == NULL && f.monitors[1].stack == NULL);
	assert(first->mon == m && second->mon == m);
	assert(first->x == m->slotx && second->x == m->slotx + m->step);
	assert(first->y == m->sloty && second->y == m->sloty);
	assert(first->w == m->slotw && second->w == m->slotw);
	assert(first->h == m->sloth && second->h == m->sloth);
	assert(first->bw == 2 && second->bw == 2);
	assert(first->tags == 1 && second->tags == 1);
	assert(!first->isfloating && !second->isfloating);
	assert(!first->isfullscreen && !second->isfullscreen);
	assert(f.updates == swapped);
	expect_server_matches(first);
	expect_server_matches(second);
}
