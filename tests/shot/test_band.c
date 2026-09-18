/* See LICENSE file for copyright and license details. */

/*
 * Outline geometry for the screenshot rubber band. Each case paints the
 * four strips onto a grid and checks containment, no overlap, perimeter
 * coverage, and that the interior hole stays empty.
 */
#include "../../shot/band.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
die(const char *message, const char *detail)
{
	fprintf(stderr, "band tests: %s%s%s\n", message, detail ? ": " : "",
		detail ? detail : "");
	exit(1);
}

static void
assert_empty(const struct shot_rect *r, const char *what)
{
	if (r->w != 0 || r->h != 0 || r->x != 0 || r->y != 0)
		die("expected empty strip", what);
}

static void
check_named_edges(void)
{
	struct shot_rect e[SHOT_BAND_EDGES];
	int n;

	n = shot_band_edges(10, 20, 100, 80, 2, e);
	if (n != 4)
		die("expected 4 strips for a large rectangle", NULL);
	if (e[0].x != 10 || e[0].y != 20 || e[0].w != 100 || e[0].h != 2)
		die("top strip", NULL);
	if (e[1].x != 10 || e[1].y != 98 || e[1].w != 100 || e[1].h != 2)
		die("bottom strip", NULL);
	if (e[2].x != 10 || e[2].y != 22 || e[2].w != 2 || e[2].h != 76)
		die("left strip", NULL);
	if (e[3].x != 108 || e[3].y != 22 || e[3].w != 2 || e[3].h != 76)
		die("right strip", NULL);
}

static void
check_invalid(void)
{
	struct shot_rect e[SHOT_BAND_EDGES];
	int n, i;
	const char *cases[] = { "w=0", "h=0", "w=-1", "h=-3" };
	int args[][4] = {
		{ 0, 0, 0, 10 },
		{ 0, 0, 10, 0 },
		{ 4, 5, -1, 8 },
		{ 4, 5, 8, -3 },
	};

	for (i = 0; i < 4; i++) {
		n = shot_band_edges(args[i][0], args[i][1], args[i][2], args[i][3],
			2, e);
		if (n != 0)
			die("invalid rectangle produced strips", cases[i]);
		assert_empty(&e[0], cases[i]);
		assert_empty(&e[1], cases[i]);
		assert_empty(&e[2], cases[i]);
		assert_empty(&e[3], cases[i]);
	}
}

static void
check_small(void)
{
	struct shot_rect e[SHOT_BAND_EDGES];
	int n;

	n = shot_band_edges(3, 4, 1, 1, 2, e);
	if (n != 1)
		die("1x1 should be a single strip", NULL);
	if (e[0].x != 3 || e[0].y != 4 || e[0].w != 1 || e[0].h != 1)
		die("1x1 covers the pixel", NULL);
	assert_empty(&e[1], "1x1 bottom");
	assert_empty(&e[2], "1x1 left");
	assert_empty(&e[3], "1x1 right");

	n = shot_band_edges(0, 0, 1, 10, 1, e);
	if (n != 3)
		die("1x10 with t=1 should be top, bottom, left", NULL);
	if (e[0].w != 1 || e[0].h != 1 || e[1].w != 1 || e[1].h != 1)
		die("1x10 caps", NULL);
	if (e[2].x != 0 || e[2].y != 1 || e[2].w != 1 || e[2].h != 8)
		die("1x10 left covers the column", NULL);
	assert_empty(&e[3], "1x10 right overlaps left, must be omitted");

	n = shot_band_edges(0, 0, 10, 1, 1, e);
	if (n != 1)
		die("10x1 is only the top strip", NULL);
	if (e[0].w != 10 || e[0].h != 1)
		die("10x1 top", NULL);
	assert_empty(&e[1], "10x1 bottom");

	n = shot_band_edges(-5, -3, 1, 3, 2, e);
	if (n != 2)
		die("1x3 t=2 should be top and a shorter bottom, no overlap", NULL);
	if (e[0].h != 2 || e[1].h != 1 || e[1].y != -1)
		die("1x3 t=2 split", NULL);
	assert_empty(&e[2], "1x3 left");
	assert_empty(&e[3], "1x3 right");
}

static int
count_nonempty(const struct shot_rect *e)
{
	int i, n = 0;

	for (i = 0; i < SHOT_BAND_EDGES; i++)
		if (e[i].w > 0 && e[i].h > 0)
			n++;
	return n;
}

static void
paint_case(int x, int y, int w, int h, int t)
{
	struct shot_rect e[SHOT_BAND_EDGES];
	unsigned char *grid;
	int i, px, py, n, ht, wt, local;
	char detail[96];

	n = shot_band_edges(x, y, w, h, t, e);
	if (n != count_nonempty(e)) {
		snprintf(detail, sizeof detail, "%d,%d %dx%d t=%d return %d",
			x, y, w, h, t, n);
		die("return count mismatches filled strips", detail);
	}
	if (w < 1 || h < 1) {
		if (n != 0)
			die("empty rectangle returned strips", NULL);
		return;
	}
	grid = calloc((size_t)w * (size_t)h, 1);
	if (!grid)
		die("out of memory", NULL);
	for (i = 0; i < SHOT_BAND_EDGES; i++) {
		int x1, y1;

		if (e[i].w < 1 || e[i].h < 1)
			continue;
		if (e[i].w < 0 || e[i].h < 0) {
			free(grid);
			die("negative strip size", NULL);
		}
		x1 = e[i].x + e[i].w;
		y1 = e[i].y + e[i].h;
		for (py = e[i].y; py < y1; py++) {
			for (px = e[i].x; px < x1; px++) {
				if (px < x || py < y || px >= x + w || py >= y + h) {
					snprintf(detail, sizeof detail,
						"pixel %d,%d outside %d,%d %dx%d t=%d",
						px, py, x, y, w, h, t);
					free(grid);
					die("strip left the rectangle", detail);
				}
				local = (py - y) * w + (px - x);
				if (grid[local]) {
					snprintf(detail, sizeof detail,
						"pixel %d,%d twice in %d,%d %dx%d t=%d",
						px, py, x, y, w, h, t);
					free(grid);
					die("overlapping strips", detail);
				}
				grid[local] = 1;
			}
		}
	}
	for (px = 0; px < w; px++) {
		if (!grid[px]) {
			snprintf(detail, sizeof detail, "top %d,%d %dx%d t=%d",
				x, y, w, h, t);
			free(grid);
			die("top edge uncovered", detail);
		}
		if (!grid[(h - 1) * w + px]) {
			snprintf(detail, sizeof detail, "bottom %d,%d %dx%d t=%d",
				x, y, w, h, t);
			free(grid);
			die("bottom edge uncovered", detail);
		}
	}
	for (py = 0; py < h; py++) {
		if (!grid[py * w]) {
			snprintf(detail, sizeof detail, "left %d,%d %dx%d t=%d",
				x, y, w, h, t);
			free(grid);
			die("left edge uncovered", detail);
		}
		if (!grid[py * w + (w - 1)]) {
			snprintf(detail, sizeof detail, "right %d,%d %dx%d t=%d",
				x, y, w, h, t);
			free(grid);
			die("right edge uncovered", detail);
		}
	}
	ht = t < 1 ? 1 : t;
	if (ht > h)
		ht = h;
	wt = t < 1 ? 1 : t;
	if (wt > w)
		wt = w;
	for (py = 0; py < h; py++) {
		for (px = 0; px < w; px++) {
			int inward = px >= wt && px < w - wt && py >= ht && py < h - ht;

			if (inward && grid[py * w + px]) {
				snprintf(detail, sizeof detail, "hole %d,%d at %d,%d t=%d",
					w, h, px, py, t);
				free(grid);
				die("interior pixel covered", detail);
			}
		}
	}
	free(grid);
}

static void
check_grid(void)
{
	static const int xs[] = { -5, 0, 7, 100 };
	static const int ys[] = { -3, 0, 11 };
	static const int ts[] = { 0, 1, 2, 3, 5, 100 };
	int xi, yi, w, h, ti;

	for (xi = 0; xi < 4; xi++)
		for (yi = 0; yi < 3; yi++)
			for (w = 1; w <= 24; w++)
				for (h = 1; h <= 24; h++)
					for (ti = 0; ti < 6; ti++)
						paint_case(xs[xi], ys[yi], w, h, ts[ti]);
}

int
main(void)
{
	check_invalid();
	check_named_edges();
	check_small();
	check_grid();
	return 0;
}
