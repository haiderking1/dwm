/* See LICENSE file for copyright and license details. */

/*
 * PNG encoder tests. Deterministic images of assorted shapes and zlib block
 * counts are encoded, checked for chunk structure, and written to the
 * directory given as argv[1]. verify_png.py then decodes each file with zlib
 * and compares every pixel against the same generator.
 */
#include "../../shot/png.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t lcg_state = 0x12345678u;

static uint32_t
lcg(void)
{
	lcg_state = (lcg_state * 1103515245u + 12345u) & 0x7fffffffu;
	return lcg_state;
}

static void
fill_solid(unsigned char *px, size_t n,
		unsigned char r, unsigned char g, unsigned char b)
{
	size_t i;

	for (i = 0; i < n; i++) {
		px[i * 3] = r;
		px[i * 3 + 1] = g;
		px[i * 3 + 2] = b;
	}
}

static void
fill_lcg(unsigned char *px, size_t n)
{
	size_t i;

	/* Every case replays the same stream so verify_png.py can regenerate it. */
	lcg_state = 0x12345678u;
	for (i = 0; i < n; i++) {
		uint32_t s = lcg();

		px[i * 3] = (unsigned char)(s & 0xffu);
		px[i * 3 + 1] = (unsigned char)((s >> 8) & 0xffu);
		px[i * 3 + 2] = (unsigned char)((s >> 16) & 0xffu);
	}
}

static void
die(const char *message, const char *detail)
{
	fprintf(stderr, "png tests: %s%s%s\n", message, detail ? ": " : "",
		detail ? detail : "");
	exit(1);
}

/* Signature, IHDR dimensions, one IDAT and a trailing IEND. */
static void
check_structure(const unsigned char *png, size_t len, unsigned int w, unsigned int h)
{
	static const unsigned char sig[8] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n' };
	uint32_t ihdr_len, idat_len, iend_len;

	if (len < 8 + 25 + 12 + 12)
		die("truncated output", NULL);
	if (memcmp(png, sig, 8) != 0)
		die("bad signature", NULL);
	ihdr_len = ((uint32_t)png[8] << 24) | ((uint32_t)png[9] << 16)
		| ((uint32_t)png[10] << 8) | png[11];
	if (ihdr_len != 13 || memcmp(png + 12, "IHDR", 4) != 0)
		die("bad IHDR", NULL);
	if (((uint32_t)png[16] << 24 | (uint32_t)png[17] << 16
			| (uint32_t)png[18] << 8 | png[19]) != w)
		die("wrong width", NULL);
	if (((uint32_t)png[20] << 24 | (uint32_t)png[21] << 16
			| (uint32_t)png[22] << 8 | png[23]) != h)
		die("wrong height", NULL);
	if (png[24] != 8 || png[25] != 2 || png[26] || png[27] || png[28])
		die("wrong IHDR fields", NULL);
	/* First IDAT length follows the IHDR chunk. */
	idat_len = ((uint32_t)png[33] << 24) | ((uint32_t)png[34] << 16)
		| ((uint32_t)png[35] << 8) | png[36];
	if (memcmp(png + 37, "IDAT", 4) != 0)
		die("missing IDAT", NULL);
	if (idat_len != len - 8 - 25 - 12 - 12)
		die("IDAT length mismatch", NULL);
	if (png[8 + 12 + 13 + 8] != 0x78 || png[8 + 12 + 13 + 9] != 0x01)
		die("bad zlib header", NULL);
	iend_len = ((uint32_t)png[len - 12] << 24) | ((uint32_t)png[len - 11] << 16)
		| ((uint32_t)png[len - 10] << 8) | png[len - 9];
	if (iend_len != 0 || memcmp(png + len - 8, "IEND", 4) != 0)
		die("missing IEND", NULL);
}

static void
write_case(const char *dir, const char *name, const unsigned char *px,
		unsigned int w, unsigned int h)
{
	char path[512];
	unsigned char *png;
	size_t png_len;
	FILE *file;
	int used;

	png = png_encode_rgb(px, w, h, &png_len);
	if (!png || png_len == 0)
		die("encode failed", name);
	check_structure(png, png_len, w, h);
	used = snprintf(path, sizeof path, "%s/%s", dir, name);
	if (used < 0 || (size_t)used >= sizeof path)
		die("path too long", name);
	file = fopen(path, "wb");
	if (!file)
		die("cannot create", path);
	if (fwrite(png, 1, png_len, file) != png_len)
		die("short write", path);
	fclose(file);
	free(png);
}

int
main(int argc, char *argv[])
{
	unsigned char *px;

	if (argc != 2) {
		fprintf(stderr, "usage: %s OUTPUT_DIR\n", argv[0]);
		return 2;
	}
	/* Rejected inputs. */
	if (png_encode_rgb(NULL, 4, 4, NULL))
		die("NULL pixels accepted", NULL);
	px = malloc(16 * 3);
	if (!px)
		die("out of memory", NULL);
	if (png_encode_rgb(px, 0, 4, NULL) || png_encode_rgb(px, 4, 0, NULL))
		die("zero dimension accepted", NULL);
	free(px);

	/* 1. single pixel */
	px = malloc(3);
	if (!px)
		die("out of memory", NULL);
	fill_solid(px, 1, 0xff, 0x7f, 0x00);
	write_case(argv[1], "one.png", px, 1, 1);
	free(px);

	/* 2. small solid */
	px = malloc((size_t)3 * 2 * 3);
	if (!px)
		die("out of memory", NULL);
	fill_solid(px, 3 * 2, 0x10, 0x20, 0x30);
	write_case(argv[1], "solid.png", px, 3, 2);
	free(px);

	/* 3. odd width, random */
	px = malloc((size_t)7 * 5 * 3);
	if (!px)
		die("out of memory", NULL);
	fill_lcg(px, 7 * 5);
	write_case(argv[1], "odd.png", px, 7, 5);
	free(px);

	/* 4. one pixel wide, tall */
	px = malloc((size_t)1 * 300 * 3);
	if (!px)
		die("out of memory", NULL);
	fill_lcg(px, 300);
	write_case(argv[1], "sparse.png", px, 1, 300);
	free(px);

	/* 5. crosses one zlib block boundary */
	px = malloc((size_t)128 * 256 * 3);
	if (!px)
		die("out of memory", NULL);
	fill_lcg(px, (size_t)128 * 256);
	write_case(argv[1], "multi.png", px, 128, 256);
	free(px);

	/* 6. maximum width, several blocks */
	px = malloc((size_t)65535 * 2 * 3);
	if (!px)
		die("out of memory", NULL);
	fill_lcg(px, (size_t)65535 * 2);
	write_case(argv[1], "wide.png", px, 65535, 2);
	free(px);

	/* 7. many blocks */
	px = malloc((size_t)513 * 513 * 3);
	if (!px)
		die("out of memory", NULL);
	fill_lcg(px, (size_t)513 * 513);
	write_case(argv[1], "big.png", px, 513, 513);
	free(px);

	printf("PNG encoder: 7 cases written\n");
	return 0;
}
