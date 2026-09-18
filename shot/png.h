/* See LICENSE file for copyright and license details. */

#ifndef SHOT_PNG_H
#define SHOT_PNG_H

#include <stddef.h>

/* Encode width * height RGB-8 pixels (3 bytes per pixel, top-down rows) as a
 * single-IDAT truecolor PNG. Stored deflate blocks keep the encoder free of
 * any compression dependency. Returns a malloc'd buffer and sets *out_len,
 * or NULL with *out_len = 0. The caller frees the buffer. */
unsigned char *png_encode_rgb(const unsigned char *rgb, unsigned int width,
                              unsigned int height, size_t *out_len);

#endif
