#ifndef WM_RESIZE_MATH_H
#define WM_RESIZE_MATH_H

#include <float.h>

/* Outer-pixel allocations ignore application size hints. Every tile first
 * receives a minimum; cfact divides only the pixels left over. */
#define RESIZE_WEIGHT_MIN 0.001f
#define RESIZE_WEIGHT_MAX 1000000.0f
#define RESIZE_MIN_PIXELS 32

static double
resize_weight(float weight)
{
	/* The ordered comparison also rejects NaN. */
	if (!(weight >= RESIZE_WEIGHT_MIN))
		return RESIZE_WEIGHT_MIN;
	return weight > RESIZE_WEIGHT_MAX ? RESIZE_WEIGHT_MAX : weight;
}

static int
resize_minimum(int extent, unsigned int count, int border)
{
	int preferred = MAX(RESIZE_MIN_PIXELS, 2 * border + 1);
	return count && extent > 0 ? MIN(preferred, extent / (int)count) : 0;
}

static int
resize_edge(unsigned int index, unsigned int count, int extent,
            int minimum, double prefix, double total)
{
	int extra = extent - (int)count * minimum;
	if (index == count)
		return extent;
	return (int)index * minimum + (int)(extra * prefix / total + 0.5);
}

static int
resize_split(int width, double factor)
{
	int minimum = MIN(RESIZE_MIN_PIXELS, width / 2);
	int split = (int)(width * factor + 0.5);
	return MAX(minimum, MIN(width - minimum, split));
}

static int
resize_pair_available(float a, float b)
{
	double sum = (double)a + b;
	return sum > 2.0 * RESIZE_WEIGHT_MIN && sum < 2.0 * RESIZE_WEIGHT_MAX;
}

static void
resize_pair(float upper, float lower, double delta, float *a, float *b)
{
	double sum = (double)upper + lower;
	double low = MAX((double)RESIZE_WEIGHT_MIN, sum - RESIZE_WEIGHT_MAX);
	double high = MIN((double)RESIZE_WEIGHT_MAX, sum - RESIZE_WEIGHT_MIN);
	double wanted = MAX(low, MIN(high, (double)upper + delta));

	if (delta == 0.0) {
		*a = upper;
		*b = lower;
		return;
	}
	/* Round the larger share first. The smaller residual retains more bits
	 * than independently rounding both shares. Keep both in the valid range. */
	if (wanted >= sum / 2.0) {
		*a = (float)wanted;
		*b = (float)MAX(low, MIN(high, sum - *a));
		*a = (float)MAX(low, MIN(high, sum - *b));
	} else {
		*b = (float)(sum - wanted);
		*a = (float)MAX(low, MIN(high, sum - *b));
		*b = (float)MAX(low, MIN(high, sum - *a));
	}
}

#endif
