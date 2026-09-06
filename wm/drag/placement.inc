/* Pure client-list operations; included after dwm's types and macros. */
static int
drag_eligible(Client *c)
{
	return c && c->mon && ISVISIBLE(c) && !c->isfloating && !c->isfullscreen
	    && c->mon->lt[c->mon->sellt]->arrange;
}

static Client *
drag_target(Monitor *mon, Client *source, int x, int y)
{
	Client *c;

	if (!mon || !mon->lt[mon->sellt]->arrange)
		return NULL;
	/* Logical geometry keeps the source's slot occupied during its preview.
	 * A floating/fullscreen window blocks the tile underneath it. */
	for (c = mon->stack; c; c = c->snext)
		if (c != source && ISVISIBLE(c) && x >= c->x && y >= c->y
		&& (long)x < (long)c->x + WIDTH(c)
		&& (long)y < (long)c->y + HEIGHT(c))
			return drag_eligible(c) ? c : NULL;
	return NULL;
}

static int
drag_swap(Client *a, Client *b)
{
	Monitor *am, *bm;
	Client **ap, **bp, **as, **bs, *next;
	unsigned int tags;

	if (a == b || !drag_eligible(a) || !drag_eligible(b))
		return 0;
	am = a->mon;
	bm = b->mon;
	for (ap = &am->clients; *ap && *ap != a; ap = &(*ap)->next);
	for (bp = &bm->clients; *bp && *bp != b; bp = &(*bp)->next);
	for (as = &am->stack; *as && *as != a; as = &(*as)->snext);
	for (bs = &bm->stack; *bs && *bs != b; bs = &(*bs)->snext);
	/* Validate every link before mutating either monitor. */
	if (!*ap || !*bp || !*as || !*bs)
		return 0;
	if (a->next == b) {
		*ap = b;
		a->next = b->next;
		b->next = a;
	} else if (b->next == a) {
		*bp = a;
		b->next = a->next;
		a->next = b;
	} else {
		next = a->next;
		*ap = b;
		*bp = a;
		a->next = b->next;
		b->next = next;
	}
	if (am != bm) {
		*as = b;
		*bs = a;
		next = a->snext;
		a->snext = b->snext;
		b->snext = next;
		a->mon = bm;
		b->mon = am;
		tags = a->tags;
		a->tags = b->tags;
		b->tags = tags;
		if (am->sel == a) am->sel = b;
		if (bm->sel == b) bm->sel = a;
	}
	return 1;
}
