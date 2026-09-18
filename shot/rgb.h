/* See LICENSE file for copyright and license details. */

#ifndef SHOT_RGB_H
#define SHOT_RGB_H

/* XGetImage on a pixmap has no visual, so red/green/blue masks come back 0.
 * Decoding those pixels then yields a black PNG even though the pixmap is
 * fine. Copy any missing mask from the screen visual used to create it. */
void shot_rgb_fill_masks(unsigned long *red, unsigned long *green,
		unsigned long *blue, unsigned long vis_red,
		unsigned long vis_green, unsigned long vis_blue);

/* Pack one ZPixmap pixel into RGB using the image masks. */
void shot_rgb_pixel(unsigned long px, unsigned long red_mask,
		unsigned long green_mask, unsigned long blue_mask,
		unsigned char out[3]);

/* Convert a 24- or 32-bpp ZPixmap buffer to packed RGB. lsb_first is
 * XImage.byte_order == LSBFirst. Returns a malloc'd width*height*3 buffer,
 * or NULL on bad input / out of memory. */
unsigned char *shot_rgb_convert(const unsigned char *data, int width,
		int height, int bytes_per_line, int bits_per_pixel,
		int lsb_first, unsigned long red_mask, unsigned long green_mask,
		unsigned long blue_mask);

/* 24/32-bpp path Print uses on the freeze pixmap: fill missing masks from
 * the screen visual, then convert. */
unsigned char *shot_rgb_from_zpixmap(const unsigned char *data, int width,
		int height, int bytes_per_line, int bits_per_pixel,
		int lsb_first, unsigned long red_mask, unsigned long green_mask,
		unsigned long blue_mask, unsigned long vis_red,
		unsigned long vis_green, unsigned long vis_blue);

#endif
