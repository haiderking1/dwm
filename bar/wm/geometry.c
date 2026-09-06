#include "geometry.h"

static int
intersects(int origin, int size, uint32_t start, uint32_t end)
{
	return start <= end && (int64_t)origin <= end &&
	       (int64_t)origin + size > start;
}

void
qs_strut_apply(QsRect m, int rootw, int rooth, const uint32_t s[12], QsRect *a)
{
	int64_t left = a->x, top = a->y;
	int64_t right = left + a->width, bottom = top + a->height, edge;
	if (s[0] && intersects(m.y, m.height, s[4], s[5])) {
		edge = s[0] < (uint32_t)rootw ? s[0] : (uint32_t)rootw;
		if (edge > left && m.x < edge && (int64_t)m.x + m.width > 0) left = edge;
	}
	if (s[1] && intersects(m.y, m.height, s[6], s[7])) {
		edge = (int64_t)rootw - (s[1] < (uint32_t)rootw ? s[1] : (uint32_t)rootw);
		if (edge < right && (int64_t)m.x + m.width > edge && m.x < rootw) right = edge;
	}
	if (s[2] && intersects(m.x, m.width, s[8], s[9])) {
		edge = s[2] < (uint32_t)rooth ? s[2] : (uint32_t)rooth;
		if (edge > top && m.y < edge && (int64_t)m.y + m.height > 0) top = edge;
	}
	if (s[3] && intersects(m.x, m.width, s[10], s[11])) {
		edge = (int64_t)rooth - (s[3] < (uint32_t)rooth ? s[3] : (uint32_t)rooth);
		if (edge < bottom && (int64_t)m.y + m.height > edge && m.y < rooth) bottom = edge;
	}
	/* Malformed or overlapping reservations must not produce X sizes <= 0. */
	if (left >= (int64_t)m.x + m.width) left = (int64_t)m.x + m.width - 1;
	if (top >= (int64_t)m.y + m.height) top = (int64_t)m.y + m.height - 1;
	if (right <= left) right = left + 1;
	if (bottom <= top) bottom = top + 1;
	a->x = left; a->y = top;
	a->width = right - left; a->height = bottom - top;
}
