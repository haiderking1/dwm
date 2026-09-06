/* Shared by layout and mouse baselines. No X events are dispatched here. */
typedef struct {
	Client *first;
	unsigned int count;
	double total;
	int minimum, extra;
} ResizeColumn;

static unsigned int
resize_count(Monitor *m)
{
	unsigned int count = 0;
	Client *c;
	for (c = nexttiled(m->clients); c; c = nexttiled(c->next))
		++count;
	return count;
}

static unsigned int
resize_masters(Monitor *m, unsigned int count)
{
	return m->nmaster > 0 ? MIN((unsigned int)m->nmaster, count) : 0;
}

static ResizeColumn
resize_column(Client *first, unsigned int count, int height)
{
	ResizeColumn column = { first, count, 0.0, 0, 0 };
	Client *c = first;
	unsigned int i;
	int border = 0;
	for (i = 0; i < count; ++i, c = nexttiled(c->next)) {
		column.total += resize_weight(c->cfact);
		border = MAX(border, c->bw);
	}
	column.minimum = resize_minimum(height, count, border);
	column.extra = column.minimum > 2 * border
	             ? height - (int)count * column.minimum : 0;
	return column;
}

static void
resize_layout_column(ResizeColumn column, int x, int y, int width, int height)
{
	Client *c = column.first;
	unsigned int i;
	double prefix = 0.0;
	int top = 0, bottom, contentw, contenth, tiley;

	for (i = 0; i < column.count; ++i, c = nexttiled(c->next)) {
		c->cfact = (float)resize_weight(c->cfact);
		prefix += c->cfact;
		bottom = resize_edge(i + 1, column.count, height,
		                     column.minimum, prefix, column.total);
		/* X requires at least one content pixel plus both borders. If the
		 * workarea cannot hold those, logical slots still partition it, but
		 * these irreducible windows overlap. Keep their bottom inside the
		 * workarea whenever even one native minimum fits in it. */
		contentw = MAX(1, width - 2 * c->bw);
		contenth = MAX(1, bottom - top - 2 * c->bw);
		tiley = y + MIN(top, MAX(0, height - contenth - 2 * c->bw));
		if (c->x != x || c->y != tiley || c->w != contentw || c->h != contenth)
			resizeclient(c, x, tiley, contentw, contenth);
		top = bottom;
	}
}

static void
resize_tile(Monitor *m)
{
	unsigned int count = resize_count(m), masters = resize_masters(m, count), i;
	Client *stack = nexttiled(m->clients);
	int width;

	if (!count || m->ww <= 0 || m->wh <= 0)
		return;
	width = masters == count ? m->ww : masters ? resize_split(m->ww, m->mfact) : 0;
	if (masters)
		resize_layout_column(resize_column(stack, masters, m->wh),
		                     m->wx, m->wy, width, m->wh);
	for (i = 0; i < masters; ++i)
		stack = nexttiled(stack->next);
	if (masters < count)
		resize_layout_column(resize_column(stack, count - masters, m->wh),
		                     m->wx + width, m->wy, m->ww - width, m->wh);
}
