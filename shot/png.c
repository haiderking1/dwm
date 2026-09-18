/* See LICENSE file for copyright and license details. */

/*
 * Dependency-free PNG encoder for screenshots. One IDAT carries a zlib
 * stream made of stored (uncompressed) deflate blocks, so the encoder needs
 * nothing beyond libc: no zlib, no libpng. Files are larger than compressed
 * output, but a 4K screenshot stays around 25 MB for a few seconds until
 * the clipboard owner releases it.
 */
#include "png.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define PNG_SIG_LEN 8
#define PNG_IHDR_DATA 13
#define PNG_CHUNK_OVERHEAD 12
#define ZLIB_MAX_BLOCK 65535u
#define ZLIB_FIXED_OVERHEAD 6    /* zlib header + adler32 trailer */
#define ZLIB_BLOCK_OVERHEAD 5    /* block header + length pair */

static uint32_t
png_crc32(const unsigned char *buf, size_t len)
{
	static uint32_t table[256];
	static int ready;
	uint32_t crc = 0xffffffffu;
	size_t i;

	if (!ready) {
		uint32_t c;
		int n, k;

		for (n = 0; n < 256; n++) {
			c = (uint32_t)n;
			for (k = 0; k < 8; k++)
				c = (c & 1u) ? 0xedb88320u ^ (c >> 1) : (c >> 1);
			table[n] = c;
		}
		ready = 1;
	}
	for (i = 0; i < len; i++)
		crc = table[(crc ^ buf[i]) & 0xffu] ^ (crc >> 8);
	return crc ^ 0xffffffffu;
}

static void
png_put32(unsigned char *out, size_t offset, uint32_t value)
{
	out[offset] = (unsigned char)(value >> 24);
	out[offset + 1] = (unsigned char)(value >> 16);
	out[offset + 2] = (unsigned char)(value >> 8);
	out[offset + 3] = (unsigned char)value;
}

static size_t
png_chunk_begin(unsigned char *out, size_t offset, const char *type, size_t len)
{
	png_put32(out, offset, (uint32_t)len);
	memcpy(out + offset + 4, type, 4);
	return offset + 8;
}

static void
png_chunk_end(unsigned char *out, size_t data, size_t len)
{
	png_put32(out, data + len, png_crc32(out + data - 4, 4 + len));
}

/* Adler-32 over the virtual filtered stream: one zero filter byte per row
 * followed by that row's RGB triplets, without materializing the stream. */
static uint32_t
png_stream_adler32(const unsigned char *rgb, unsigned int width, unsigned int height)
{
	uint32_t a = 1, b = 0;
	size_t rowbytes = 1u + (size_t)width * 3u;
	unsigned int y;
	size_t x;

	for (y = 0; y < height; y++) {
		const unsigned char *row = rgb + (size_t)y * width * 3u;

		a = (a + 0) % 65521u;
		b = (b + a) % 65521u;
		for (x = 0; x < rowbytes - 1u; x++) {
			a = (a + row[x]) % 65521u;
			b = (b + a) % 65521u;
		}
	}
	return (b << 16) | a;
}

unsigned char *
png_encode_rgb(const unsigned char *rgb, unsigned int width, unsigned int height,
		size_t *out_len)
{
	unsigned char *out;
	unsigned char ihdr[PNG_IHDR_DATA];
	size_t rowbytes, raw, blocks, zlen, total, offset, block, copied;
	uint32_t adler;

	if (out_len)
		*out_len = 0;
	if (!rgb || width == 0 || height == 0)
		return NULL;
	rowbytes = 1u + (size_t)width * 3u;
	if (rowbytes > (size_t)-1 / height)
		return NULL;
	raw = rowbytes * height;
	blocks = (raw + ZLIB_MAX_BLOCK - 1) / ZLIB_MAX_BLOCK;
	zlen = ZLIB_FIXED_OVERHEAD + blocks * ZLIB_BLOCK_OVERHEAD + raw;
	total = PNG_SIG_LEN + PNG_CHUNK_OVERHEAD + PNG_IHDR_DATA
		+ PNG_CHUNK_OVERHEAD + zlen + PNG_CHUNK_OVERHEAD;
	out = malloc(total);
	if (!out)
		return NULL;

	memcpy(out, "\211PNG\r\n\032\n", PNG_SIG_LEN);
	png_put32(ihdr, 0, (uint32_t)width);
	png_put32(ihdr, 4, (uint32_t)height);
	ihdr[8] = 8;  /* bit depth */
	ihdr[9] = 2;  /* color type: truecolor RGB */
	ihdr[10] = 0; /* compression method */
	ihdr[11] = 0; /* filter method */
	ihdr[12] = 0; /* no interlace */
	offset = png_chunk_begin(out, PNG_SIG_LEN, "IHDR", PNG_IHDR_DATA);
	memcpy(out + offset, ihdr, PNG_IHDR_DATA);
	png_chunk_end(out, offset, PNG_IHDR_DATA);
	offset += PNG_IHDR_DATA + 4; /* data, then CRC, then the next header */

	offset = png_chunk_begin(out, offset, "IDAT", zlen);

	/* zlib header: CM=8, CINFO=7, FLEVEL=0, FDICT=0, FCHECK for 0x78 0x01. */
	out[offset] = 0x78;
	out[offset + 1] = 0x01;
	offset += 2;

	for (block = 0; block < blocks; block++) {
		size_t start = block * ZLIB_MAX_BLOCK;
		size_t len = raw - start;
		unsigned char bfinal;

		if (len > ZLIB_MAX_BLOCK)
			len = ZLIB_MAX_BLOCK;
		bfinal = (unsigned char)(start + len == raw);
		out[offset] = bfinal;
		out[offset + 1] = (unsigned char)(len & 0xffu);
		out[offset + 2] = (unsigned char)(len >> 8);
		out[offset + 3] = (unsigned char)(~len & 0xffu);
		out[offset + 4] = (unsigned char)((~len >> 8) & 0xffu);
		offset += ZLIB_BLOCK_OVERHEAD;
		/* Stored data walks the filtered scanlines block by block. */
		copied = 0;
		while (copied < len) {
			size_t pos = start + copied;
			size_t row = pos / rowbytes;
			size_t col = pos % rowbytes;
			size_t take = rowbytes - col;

			if (take > len - copied)
				take = len - copied;
			if (col == 0) {
				out[offset + copied] = 0; /* filter type None */
				if (take > 1)
					memcpy(out + offset + copied + 1,
						rgb + row * (rowbytes - 1u), take - 1);
			} else {
				memcpy(out + offset + copied,
					rgb + row * (rowbytes - 1u) + col - 1u, take);
			}
			copied += take;
		}
		offset += len;
	}

	adler = png_stream_adler32(rgb, width, height);
	png_put32(out, offset, adler);
	offset += 4;
	png_chunk_end(out, offset - zlen, zlen);
	offset += 4; /* the CRC just written, then the IEND header */

	offset = png_chunk_begin(out, offset, "IEND", 0);
	png_chunk_end(out, offset, 0);

	if (out_len)
		*out_len = total;
	return out;
}
