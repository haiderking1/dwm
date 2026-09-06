/* A float pair cannot always represent its old sum exactly, particularly
 * after one share hits 0.001. Check the actual rounded slots, not just an
 * epsilon on that sum: a tiny error at a half-pixel tie can move a sibling. */
static int
resize_pair_fits(const TileResize *r, float a, float b)
{
	unsigned int start = r->upper < r->masters ? 0 : r->masters;
	unsigned int count = r->upper < r->masters ? r->masters : r->count - start;
	unsigned int i;
	double total = 0.0, prefix = 0.0;
	int bottom;

	if (a < RESIZE_WEIGHT_MIN || a > RESIZE_WEIGHT_MAX
	|| b < RESIZE_WEIGHT_MIN || b > RESIZE_WEIGHT_MAX)
		return 0;
	for (i = start; i < start + count; ++i)
		total += i == r->upper ? a : i == r->lower ? b : r->slots[i].weight;
	for (i = start; i < start + count; ++i) {
		const ResizeSlot *s = &r->slots[i];
		prefix += i == r->upper ? a : i == r->lower ? b : s->weight;
		bottom = resize_edge(i - start + 1, count, r->wh, r->minimum, prefix, total);
		if (i != r->upper && bottom != s->y - r->wy + s->h + 2 * s->border)
			return 0;
	}
	return 1;
}

static void
resize_stable_pair(const TileResize *r, double delta, float *a, float *b)
{
	const int directions[] = { 0, -1, 1 };
	float wanted_a, wanted_b, next_a, next_b;
	unsigned int i, j;

	resize_pair(r->upperweight, r->lowerweight, delta, &wanted_a, &wanted_b);
	/* Try adjacent representable shares when rounding the residual alone
	 * crosses another tile's rounding threshold. This is subpixel correction,
	 * not a change to any sibling's weight or the drag's pixel baseline. */
	for (i = 0; i < 3; ++i)
	for (j = 0; j < 3; ++j) {
		next_a = (float)(wanted_a + (double)directions[i] * wanted_a * FLT_EPSILON);
		next_b = (float)(wanted_b + (double)directions[j] * wanted_b * FLT_EPSILON);
		if (resize_pair_fits(r, next_a, next_b)) {
			*a = next_a; *b = next_b;
			return;
		}
	}
	/* If the float grid has no suitable pair, keep the last valid boundary
	 * rather than moving a sibling. A later pointer position can still fit. */
	*a = r->slots[r->upper].weight;
	*b = r->slots[r->lower].weight;
}
