/* See LICENSE file for copyright and license details. */

/*
 * Freeze-pixmap RGB conversion. Print crops from a pixmap, and XGetImage on
 * a pixmap leaves the channel masks at 0. Decoding that without filling the
 * masks from the screen visual is a black PNG of the right size.
 */
#include "../../shot/rgb.h"

#include <stdio.h>
#include <stdlib.h>

static void
die(const char *message, const char *detail)
{
	fprintf(stderr, "rgb tests: %s%s%s\n", message, detail ? ": " : "",
		detail ? detail : "");
	exit(1);
}

static void
expect_rgb(const unsigned char *got, unsigned char r, unsigned char g,
		unsigned char b, const char *what)
{
	if (!got)
		die("NULL convert", what);
	if (got[0] != r || got[1] != g || got[2] != b) {
		char detail[128];

		snprintf(detail, sizeof detail, "%s got %u,%u,%u want %u,%u,%u",
			what, got[0], got[1], got[2], r, g, b);
		die("pixel", detail);
	}
}

static void
check_fill_masks(void)
{
	unsigned long red, green, blue;

	red = green = blue = 0;
	shot_rgb_fill_masks(&red, &green, &blue, 0x00ff0000UL, 0x0000ff00UL,
		0x000000ffUL);
	if (red != 0x00ff0000UL || green != 0x0000ff00UL || blue != 0x000000ffUL)
		die("zero masks were not filled from the visual", NULL);

	red = 0x000000ffUL;
	green = 0x0000ff00UL;
	blue = 0x00ff0000UL;
	shot_rgb_fill_masks(&red, &green, &blue, 0x00ff0000UL, 0x0000ff00UL,
		0x000000ffUL);
	if (red != 0x000000ffUL || green != 0x0000ff00UL || blue != 0x00ff0000UL)
		die("existing window masks were overwritten", NULL);

	red = 0;
	green = 0x0000ff00UL;
	blue = 0;
	shot_rgb_fill_masks(&red, &green, &blue, 0x00ff0000UL, 0x0000ff00UL,
		0x000000ffUL);
	if (red != 0x00ff0000UL || green != 0x0000ff00UL || blue != 0x000000ffUL)
		die("partial zero masks were not filled", NULL);

	shot_rgb_fill_masks(NULL, NULL, NULL, 1, 2, 3);
}

static void
check_black_without_masks(void)
{
	unsigned char px[4] = { 0xcc, 0xbb, 0xaa, 0x00 };
	unsigned char rgb[3];
	unsigned char *out;

	/* Typical TrueColor 0x00AABBCC stored LSBFirst as CC BB AA 00. */
	shot_rgb_pixel(0x00aabbccUL, 0, 0, 0, rgb);
	expect_rgb(rgb, 0, 0, 0, "zero masks pixel");

	out = shot_rgb_convert(px, 1, 1, 4, 32, 1, 0, 0, 0);
	expect_rgb(out, 0, 0, 0, "pixmap GetImage without visual masks");
	free(out);
}

static void
check_truecolor_after_fill(void)
{
	unsigned long red = 0, green = 0, blue = 0;
	unsigned char px[4] = { 0xcc, 0xbb, 0xaa, 0x00 };
	unsigned char rgb[3];
	unsigned char *out;

	shot_rgb_fill_masks(&red, &green, &blue, 0x00ff0000UL, 0x0000ff00UL,
		0x000000ffUL);
	shot_rgb_pixel(0x00aabbccUL, red, green, blue, rgb);
	expect_rgb(rgb, 0xaa, 0xbb, 0xcc, "filled masks pixel");

	out = shot_rgb_convert(px, 1, 1, 4, 32, 1, red, green, blue);
	expect_rgb(out, 0xaa, 0xbb, 0xcc, "pixmap GetImage after fill");
	free(out);

	/* Same glue Print uses: zero XImage masks plus the screen visual. */
	out = shot_rgb_from_zpixmap(px, 1, 1, 4, 32, 1, 0, 0, 0,
		0x00ff0000UL, 0x0000ff00UL, 0x000000ffUL);
	expect_rgb(out, 0xaa, 0xbb, 0xcc, "Print freeze crop");
	free(out);

	/* Window GetImage already has masks; the visual must not replace them. */
	out = shot_rgb_from_zpixmap(px, 1, 1, 4, 32, 1, 0x000000ffUL,
		0x0000ff00UL, 0x00ff0000UL, 0x00ff0000UL, 0x0000ff00UL,
		0x000000ffUL);
	expect_rgb(out, 0xcc, 0xbb, 0xaa, "window masks kept");
	free(out);
}

static void
check_rgb_in_low_bytes(void)
{
	/* red in the low byte: 0x0000BBGGRR, LSBFirst RR GG BB 00. */
	unsigned char px[4] = { 0x11, 0x22, 0x33, 0x00 };
	unsigned char *out;

	out = shot_rgb_convert(px, 1, 1, 4, 32, 1, 0x000000ffUL, 0x0000ff00UL,
		0x00ff0000UL);
	expect_rgb(out, 0x11, 0x22, 0x33, "BGRX visual");
	free(out);
}

static void
check_24bpp_and_msb(void)
{
	unsigned char packed[3] = { 0xcc, 0xbb, 0xaa };
	unsigned char packed_msb[3] = { 0xaa, 0xbb, 0xcc };
	unsigned char msb[4] = { 0x00, 0xaa, 0xbb, 0xcc };
	unsigned char *out;

	out = shot_rgb_convert(packed, 1, 1, 3, 24, 1, 0x00ff0000UL, 0x0000ff00UL,
		0x000000ffUL);
	expect_rgb(out, 0xaa, 0xbb, 0xcc, "24-bpp LSB");
	free(out);

	out = shot_rgb_convert(packed_msb, 1, 1, 3, 24, 0, 0x00ff0000UL,
		0x0000ff00UL, 0x000000ffUL);
	expect_rgb(out, 0xaa, 0xbb, 0xcc, "24-bpp MSB");
	free(out);

	out = shot_rgb_convert(msb, 1, 1, 4, 32, 0, 0x00ff0000UL, 0x0000ff00UL,
		0x000000ffUL);
	expect_rgb(out, 0xaa, 0xbb, 0xcc, "32-bpp MSB");
	free(out);
}

static void
check_padded_rows(void)
{
	/* width 1, 32-bpp, 8-byte stride, two rows. */
	unsigned char px[16] = {
		0x01, 0x02, 0x03, 0x00, 0xff, 0xff, 0xff, 0xff,
		0x04, 0x05, 0x06, 0x00, 0xff, 0xff, 0xff, 0xff,
	};
	unsigned char *out;

	out = shot_rgb_convert(px, 1, 2, 8, 32, 1, 0x00ff0000UL, 0x0000ff00UL,
		0x000000ffUL);
	if (!out)
		die("padded convert failed", NULL);
	expect_rgb(out, 0x03, 0x02, 0x01, "padded row 0");
	expect_rgb(out + 3, 0x06, 0x05, 0x04, "padded row 1");
	free(out);
}

static void
check_channel_widths(void)
{
	unsigned char rgb[3];

	/* 10-bit red in the high bits of a 32-bit pixel: take the top 8. */
	shot_rgb_pixel(0x3ff00000UL, 0x3ff00000UL, 0, 0, rgb);
	expect_rgb(rgb, 0xff, 0, 0, "10-bit red");

	/* 5-bit red 0x1f scales to 255. */
	shot_rgb_pixel(0x1fUL, 0x1fUL, 0, 0, rgb);
	expect_rgb(rgb, 0xff, 0, 0, "5-bit red");

	shot_rgb_pixel(0UL, 0x1fUL, 0, 0, rgb);
	expect_rgb(rgb, 0, 0, 0, "5-bit red zero");
}

static void
check_reject(void)
{
	unsigned char px[4] = { 1, 2, 3, 4 };

	if (shot_rgb_convert(NULL, 1, 1, 4, 32, 1, 0xff0000UL, 0xff00UL, 0xffUL))
		die("NULL data accepted", NULL);
	if (shot_rgb_convert(px, 0, 1, 4, 32, 1, 0xff0000UL, 0xff00UL, 0xffUL))
		die("zero width accepted", NULL);
	if (shot_rgb_convert(px, 1, 0, 4, 32, 1, 0xff0000UL, 0xff00UL, 0xffUL))
		die("zero height accepted", NULL);
	if (shot_rgb_convert(px, 1, 1, 4, 16, 1, 0xff0000UL, 0xff00UL, 0xffUL))
		die("16-bpp accepted", NULL);
	if (shot_rgb_convert(px, 1, 1, 3, 32, 1, 0xff0000UL, 0xff00UL, 0xffUL))
		die("short stride accepted", NULL);
	shot_rgb_pixel(0x00aabbccUL, 0xff0000UL, 0xff00UL, 0xffUL, NULL);
}

int
main(void)
{
	check_fill_masks();
	check_black_without_masks();
	check_truecolor_after_fill();
	check_rgb_in_low_bytes();
	check_24bpp_and_msb();
	check_padded_rows();
	check_channel_widths();
	check_reject();
	printf("rgb conversion: 8 checks passed\n");
	return 0;
}
