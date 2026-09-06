/* Always validate all slots before using any old pixel or weight baseline. */
static int
resize_update(TileResize *r, int x, int y)
{
	Client *source, *upper, *lower;
	Monitor *m;
	float factor, a, b;
	int changed = 0;
	double next;

	if (!resize_topology(r))
		return 0;
	source = wintoclient(r->source);
	m = source->mon;
	if (r->horizontal) {
		next = ((double)r->split + (double)x - r->startx) / r->ww;
		factor = x == r->startx ? r->startfactor : (float)MAX(0.05, MIN(0.95, next));
		if (m->mfact != factor) {
			m->mfact = factor;
			changed = 1;
		}
	}
	if (r->vertical) {
		upper = wintoclient(r->slots[r->upper].window);
		lower = wintoclient(r->slots[r->lower].window);
		resize_stable_pair(r,
		            ((double)y - r->starty) * r->total / r->extra, &a, &b);
		if (upper->cfact != a || lower->cfact != b) {
			upper->cfact = a;
			lower->cfact = b;
			changed = 1;
		}
	}
	if (changed) {
		/* Membership and stacking are unchanged during a resize frame. */
		resize_tile(m);
		resize_remember(r, m);
	}
	return 1;
}
