#include "wallpaper.h"
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char previous_file[64];
static int image_error;
static int trap_error(Display *display, XErrorEvent *event) {
    (void)display; (void)event; image_error = 1; return 0;
}
void bridge_wallpaper_cleanup(void) {
    if (previous_file[0]) unlink(previous_file);
    previous_file[0] = 0;
}
static unsigned char channel(unsigned long pixel, unsigned long mask) {
    if (!mask) return 0;
    while (!(mask & 1)) { mask >>= 1; pixel >>= 1; }
    return (unsigned char)(((pixel & mask) * 255UL) / mask);
}
int bridge_wallpaper_emit(Display *display, Window root) {
    Atom actual, property = XInternAtom(display, "_XROOTPMAP_ID", False);
    unsigned char *data = NULL, *row = NULL;
    unsigned long count, extra;
    int format, x, y, fd = -1, good = 0;
    unsigned int width, height, border, depth, column, line;
    Window ignored;
    Pixmap pixmap = None;
    XImage *image = NULL;
    XErrorHandler old_handler;
    Visual *visual = DefaultVisual(display, DefaultScreen(display));
    FILE *file = NULL;
    char path[] = "/tmp/dwm-bar-wallpaper-XXXXXX";

    if (XGetWindowProperty(display, root, property, 0, 1, False, XA_PIXMAP,
            &actual, &format, &count, &extra, &data) == Success &&
            actual == XA_PIXMAP && format == 32 && count == 1 && !extra)
        pixmap = *(Pixmap *)data;
    XFree(data);
    if (pixmap == None) goto unavailable;
    /* A wallpaper setter may free its old pixmap while we are reading it. */
    XSync(display, False);
    image_error = 0;
    old_handler = XSetErrorHandler(trap_error);
    if (XGetGeometry(display, pixmap, &ignored, &x, &y, &width, &height, &border, &depth)
            && width && height && (unsigned long)width * height <= 16777216UL)
        image = XGetImage(display, pixmap, 0, 0, width, height, AllPlanes, ZPixmap);
    XSync(display, False);
    XSetErrorHandler(old_handler);
    if (image_error || !image) goto unavailable;
    if (!image->red_mask) image->red_mask = visual->red_mask;
    if (!image->green_mask) image->green_mask = visual->green_mask;
    if (!image->blue_mask) image->blue_mask = visual->blue_mask;
    row = malloc((size_t)width * 3);
    if (!row || (fd = mkstemp(path)) < 0) goto unavailable;
    file = fdopen(fd, "wb");
    if (!file) { close(fd); unlink(path); goto unavailable; }
    good = fprintf(file, "P6\n%u %u\n255\n", width, height) > 0;
    for (line = 0; good && line < height; line++) {
        for (column = 0; column < width; column++) {
            unsigned long pixel = XGetPixel(image, column, line);
            row[column * 3] = channel(pixel, image->red_mask);
            row[column * 3 + 1] = channel(pixel, image->green_mask);
            row[column * 3 + 2] = channel(pixel, image->blue_mask);
        }
        good = fwrite(row, 3, width, file) == width;
    }
    if (fclose(file)) good = 0;
    file = NULL;
    if (!good) { unlink(path); goto unavailable; }
    free(row);
    XDestroyImage(image);
    bridge_wallpaper_cleanup();
    memcpy(previous_file, path, sizeof path);
    return printf("{\"type\":\"wallpaper\",\"path\":\"%s\",\"width\":%d,\"height\":%d}\n",
            path, DisplayWidth(display, DefaultScreen(display)),
            DisplayHeight(display, DefaultScreen(display))) >= 0 && fflush(stdout) == 0;
unavailable:
    free(row);
    if (image) XDestroyImage(image);
    /* Keep the last valid image through a wallpaper setter's transient gap. */
    if (previous_file[0]) return 1;
    return printf("{\"type\":\"wallpaper\",\"path\":null}\n") >= 0 && fflush(stdout) == 0;
}
