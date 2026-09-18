/* See LICENSE file for copyright and license details. */

#ifndef SHOT_BAND_H
#define SHOT_BAND_H

enum { SHOT_BAND_EDGES = 4 };

struct shot_rect {
	int x, y, w, h;
};

/* Fill out[4] with top, bottom, left, right strips of thickness t that
 * outline the inclusive pixel rectangle (x, y, w, h). Thickness < 1 is
 * treated as 1. A strip that would be empty has w = h = 0. Corners belong
 * to the horizontal strips, so the four rectangles never overlap.
 * Returns the number of non-empty strips. */
int shot_band_edges(int x, int y, int w, int h, int t,
		struct shot_rect out[SHOT_BAND_EDGES]);

#endif
