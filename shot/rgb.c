/* See LICENSE file for copyright and license details. */

#include "rgb.h"

#include <stddef.h>
#include <stdlib.h>

static void
shot_channel_bits(unsigned long mask, unsigned int *shift, unsigned int *bits)
{
	unsigned long m = mask;
	unsigned int s = 0, b = 0;

	if (!m) {
		*shift = 0;
		*bits = 0;
		return;
	}
	while (!(m & 1u)) {
		m >>= 1;
		s++;
	}
	while (m & 1u) {
		m >>= 1;
		b++;
	}
	*shift = s;
	*bits = b;
}

static unsigned char
shot_channel(unsigned long px, unsigned int shift, unsigned int bits)
{
	unsigned long max, v;

	if (!bits)
		return 0;
	max = (1UL << bits) - 1;
	v = (px >> shift) & max;
	if (bits >= 8)
		return (unsigned char)(v >> (bits - 8));
	return (unsigned char)((v * 255 + max / 2) / max);
}

static unsigned long
shot_read_pixel(const unsigned char *row, unsigned int x, unsigned int bpp,
		int lsb_first)
{
	const unsigned char *p = row + (size_t)x * (bpp / 8);

	if (lsb_first) {
		switch (bpp) {
		case 32: return (unsigned long)p[0] | ((unsigned long)p[1] << 8)
				| ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
		case 24: return (unsigned long)p[0] | ((unsigned long)p[1] << 8)
				| ((unsigned long)p[2] << 16);
		}
	} else {
		switch (bpp) {
		case 32: return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16)
				| ((unsigned long)p[2] << 8) | (unsigned long)p[3];
		case 24: return ((unsigned long)p[0] << 16) | ((unsigned long)p[1] << 8)
				| (unsigned long)p[2];
		}
	}
	return 0;
}

void
shot_rgb_fill_masks(unsigned long *red, unsigned long *green,
		unsigned long *blue, unsigned long vis_red,
		unsigned long vis_green, unsigned long vis_blue)
{
	if (red && !*red)
		*red = vis_red;
	if (green && !*green)
		*green = vis_green;
	if (blue && !*blue)
		*blue = vis_blue;
}

void
shot_rgb_pixel(unsigned long px, unsigned long red_mask,
		unsigned long green_mask, unsigned long blue_mask,
		unsigned char out[3])
{
	unsigned int rs, gs, bs, rb, gb, bb;

	if (!out)
		return;
	shot_channel_bits(red_mask, &rs, &rb);
	shot_channel_bits(green_mask, &gs, &gb);
	shot_channel_bits(blue_mask, &bs, &bb);
	out[0] = shot_channel(px, rs, rb);
	out[1] = shot_channel(px, gs, gb);
	out[2] = shot_channel(px, bs, bb);
}

unsigned char *
shot_rgb_convert(const unsigned char *data, int width, int height,
		int bytes_per_line, int bits_per_pixel, int lsb_first,
		unsigned long red_mask, unsigned long green_mask,
		unsigned long blue_mask)
{
	unsigned char *out, *dst;
	unsigned int x, y, bpp, min_stride;
	unsigned int rs, gs, bs, rb, gb, bb;

	if (!data || width < 1 || height < 1)
		return NULL;
	if (bits_per_pixel != 24 && bits_per_pixel != 32)
		return NULL;
	bpp = (unsigned int)bits_per_pixel;
	if ((unsigned int)width > (unsigned int)-1 / (bpp / 8))
		return NULL;
	min_stride = (unsigned int)width * (bpp / 8);
	if (bytes_per_line < (int)min_stride)
		return NULL;
	if ((size_t)width > (size_t)-1 / 3 / (size_t)height)
		return NULL;
	out = malloc((size_t)width * (size_t)height * 3);
	if (!out)
		return NULL;
	shot_channel_bits(red_mask, &rs, &rb);
	shot_channel_bits(green_mask, &gs, &gb);
	shot_channel_bits(blue_mask, &bs, &bb);
	for (y = 0; y < (unsigned int)height; y++) {
		const unsigned char *row = data + (size_t)y * (size_t)bytes_per_line;

		dst = out + (size_t)y * (size_t)width * 3;
		for (x = 0; x < (unsigned int)width; x++) {
			unsigned long px = shot_read_pixel(row, x, bpp, lsb_first);

			dst[0] = shot_channel(px, rs, rb);
			dst[1] = shot_channel(px, gs, gb);
			dst[2] = shot_channel(px, bs, bb);
			dst += 3;
		}
	}
	return out;
}

unsigned char *
shot_rgb_from_zpixmap(const unsigned char *data, int width, int height,
		int bytes_per_line, int bits_per_pixel, int lsb_first,
		unsigned long red_mask, unsigned long green_mask,
		unsigned long blue_mask, unsigned long vis_red,
		unsigned long vis_green, unsigned long vis_blue)
{
	shot_rgb_fill_masks(&red_mask, &green_mask, &blue_mask, vis_red,
		vis_green, vis_blue);
	return shot_rgb_convert(data, width, height, bytes_per_line,
		bits_per_pixel, lsb_first, red_mask, green_mask, blue_mask);
}
