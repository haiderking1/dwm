/* See LICENSE file for copyright and license details. */

#include "band.h"

int
shot_band_edges(int x, int y, int w, int h, int t,
		struct shot_rect out[SHOT_BAND_EDGES])
{
	int i, ht, wt, inner_h, n = 0;

	for (i = 0; i < SHOT_BAND_EDGES; i++) {
		out[i].x = 0;
		out[i].y = 0;
		out[i].w = 0;
		out[i].h = 0;
	}
	if (w < 1 || h < 1)
		return 0;
	if (t < 1)
		t = 1;
	ht = t < h ? t : h;
	wt = t < w ? t : w;
	out[0].x = x;
	out[0].y = y;
	out[0].w = w;
	out[0].h = ht;
	n++;
	inner_h = 0;
	if (h > ht) {
		int bh = h - ht;

		if (bh > t)
			bh = t;
		out[1].x = x;
		out[1].y = y + h - bh;
		out[1].w = w;
		out[1].h = bh;
		n++;
		inner_h = h - ht - bh;
	}
	if (inner_h < 1)
		return n;
	out[2].x = x;
	out[2].y = y + ht;
	out[2].w = wt;
	out[2].h = inner_h;
	n++;
	if (w > 2 * wt) {
		out[3].x = x + w - wt;
		out[3].y = y + ht;
		out[3].w = wt;
		out[3].h = inner_h;
		n++;
	} else if (w > wt) {
		out[3].x = x + wt;
		out[3].y = y + ht;
		out[3].w = w - wt;
		out[3].h = inner_h;
		n++;
	}
	return n;
}
