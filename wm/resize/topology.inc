typedef struct {
	Window window;
	float weight;
	int border, x, y, w, h;
} ResizeSlot;

typedef struct {
	Window source;
	Monitor *monitor; /* Identity only; never dereferenced after dispatch. */
	ResizeSlot *slots;
	unsigned int count, masters, tags, seltags, sellt;
	int wx, wy, ww, wh, nmaster;
	float factor, startfactor;
	unsigned int upper, lower;
	float upperweight, lowerweight;
	double total;
	int minimum, extra, horizontal, vertical, split, startx, starty;
} TileResize;

static int
resize_eligible(Client *c)
{
	return c && c->mon && ISVISIBLE(c) && !c->isfloating && !c->isfullscreen
	    && c->mon->lt[c->mon->sellt]->arrange == tile;
}

static void
resize_remember(TileResize *r, Monitor *m)
{
	Client *c;
	unsigned int i = 0;
	for (c = nexttiled(m->clients); c; c = nexttiled(c->next), ++i) {
		ResizeSlot *s = &r->slots[i];
		*s = (ResizeSlot){ c->win, c->cfact, c->bw, c->x, c->y, c->w, c->h };
	}
	r->factor = m->mfact;
}

static int
resize_topology(const TileResize *r)
{
	Client *c = wintoclient(r->source);
	Monitor *m;
	unsigned int i = 0;

	if (!resize_eligible(c) || c->mon != r->monitor)
		return 0;
	m = c->mon;
	if (m->sel != c || m->wx != r->wx || m->wy != r->wy
	|| m->ww != r->ww || m->wh != r->wh || m->nmaster != r->nmaster
	|| m->seltags != r->seltags || m->sellt != r->sellt
	|| m->tagset[m->seltags] != r->tags || m->mfact != r->factor)
		return 0;
	for (c = nexttiled(m->clients); c; c = nexttiled(c->next), ++i) {
		const ResizeSlot *s;
		if (i == r->count)
			return 0;
		s = &r->slots[i];
		if (c->win != s->window || c->cfact != s->weight || c->bw != s->border
		|| c->x != s->x || c->y != s->y || c->w != s->w || c->h != s->h
		|| c->isfullscreen)
			return 0;
	}
	return i == r->count;
}

static int
resize_begin(TileResize *r, Client *source, int x, int y)
{
	Monitor *m = source->mon;
	Client *c, *first = NULL;
	ResizeColumn column;
	unsigned int i, selected = 0, start, count;

	*r = (TileResize){0};
	if (!resize_eligible(source) || m->ww <= 0 || m->wh <= 0)
		return 0;
	r->count = resize_count(m);
	r->masters = resize_masters(m, r->count);
	for (i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), ++i)
		if (c == source)
			selected = i;
	start = selected < r->masters ? 0 : r->masters;
	count = selected < r->masters ? r->masters : r->count - r->masters;
	for (i = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), ++i)
		if (i == start) {
			first = c;
			break;
		}
	column = resize_column(first, count, m->wh);
	r->horizontal = r->masters && r->masters < r->count
	             && m->ww > 2 * RESIZE_MIN_PIXELS;
	r->vertical = count > 1 && column.extra > 0;
	if (!r->horizontal && !r->vertical)
		return 0;
	r->slots = calloc(r->count, sizeof(*r->slots));
	if (!r->slots)
		return 0;
	r->source = source->win;
	r->monitor = m;
	r->wx = m->wx; r->wy = m->wy; r->ww = m->ww; r->wh = m->wh;
	r->nmaster = m->nmaster;
	r->seltags = m->seltags; r->sellt = m->sellt;
	r->tags = m->tagset[m->seltags];
	r->startx = x; r->starty = y;
	r->startfactor = m->mfact;
	r->split = resize_split(m->ww, m->mfact);
	r->total = column.total;
	r->minimum = column.minimum;
	r->extra = column.extra;
	resize_remember(r, m);
	if (r->vertical) {
		int above = selected > start
		         && resize_pair_available(r->slots[selected - 1].weight, source->cfact);
		int below = selected + 1 < start + count
		         && resize_pair_available(source->cfact, r->slots[selected + 1].weight);
		/* Only internal boundaries in this column are candidates. At either
		 * end there is one candidate, regardless of which edge was clicked. */
		if (!above && !below)
			r->vertical = 0;
		else if (!above)
			r->upper = selected;
		else if (!below)
			r->upper = selected - 1;
		else {
			double top = (double)y - source->y;
			double bottom = (double)y - source->y - HEIGHT(source);
			if (top < 0) top = -top;
			if (bottom < 0) bottom = -bottom;
			r->upper = top < bottom ? selected - 1 : selected;
		}
	}
	if (r->vertical) {
		r->lower = r->upper + 1;
		r->upperweight = r->slots[r->upper].weight;
		r->lowerweight = r->slots[r->lower].weight;
	}
	if (!r->horizontal && !r->vertical) {
		free(r->slots);
		r->slots = NULL;
		return 0;
	}
	return 1;
}
