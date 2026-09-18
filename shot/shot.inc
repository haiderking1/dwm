/*
 * Screenshot entry points, root capture and PNG output. Pixels come from the
 * root window with XGetImage, which reads whatever is on screen, games
 * included: no unredirect step, no compositor, no helper process. dwm then
 * saves the PNG under shotdir and serves it on CLIPBOARD itself.
 */
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

static const char shot_uri_prefix[] = "file://";

static void
shot_mkdirs(const char *dir)
{
	char *copy, *p;

	copy = strdup(dir);
	if (!copy)
		return;
	for (p = copy + 1; *p; p++) {
		if (*p == '/') {
			*p = '\0';
			mkdir(copy, 0755);
			*p = '/';
		}
	}
	mkdir(copy, 0755);
	free(copy);
}

static char *
shot_save(const unsigned char *png, size_t len)
{
	char path[512], name[64], *dup = NULL;
	time_t now;
	struct tm *tm;
	FILE *file;
	int i, used;

	shot_mkdirs(shotdir);
	now = time(NULL);
	tm = localtime(&now);
	if (!tm)
		return NULL;
	strftime(name, sizeof name, "shot-%Y%m%d-%H%M%S", tm);
	used = snprintf(path, sizeof path, "%s/%s.png", shotdir, name);
	if (used < 0 || (size_t)used >= sizeof path)
		return NULL;
	/* Two shots within one second get a counter suffix. */
	for (i = 1; access(path, F_OK) == 0 && i < 1000; i++) {
		used = snprintf(path, sizeof path, "%s/%s-%d.png", shotdir, name, i);
		if (used < 0 || (size_t)used >= sizeof path)
			return NULL;
	}
	file = fopen(path, "wb");
	if (!file) {
		fprintf(stderr, "dwm: shot: cannot write %s\n", path);
		return NULL;
	}
	if (fwrite(png, 1, len, file) != len) {
		fprintf(stderr, "dwm: shot: short write to %s\n", path);
		fclose(file);
		return NULL;
	}
	fclose(file);
	dup = strdup(path);
	if (!dup)
		fprintf(stderr, "dwm: shot: out of memory\n");
	return dup;
}

/* Convert any ZPixmap layout to packed RGB. Pixmap GetImage leaves the
 * channel masks at 0; fill them from the screen visual first. */
static unsigned char *
shot_rgb_from_image(XImage *img)
{
	Visual *vis = DefaultVisual(dpy, screen);
	unsigned char *out, *dst;
	unsigned int x, y;

	if (img->bits_per_pixel == 32 || img->bits_per_pixel == 24)
		return shot_rgb_from_zpixmap((const unsigned char *)img->data,
			img->width, img->height, img->bytes_per_line,
			img->bits_per_pixel, img->byte_order == LSBFirst,
			img->red_mask, img->green_mask, img->blue_mask,
			vis->red_mask, vis->green_mask, vis->blue_mask);
	shot_rgb_fill_masks(&img->red_mask, &img->green_mask, &img->blue_mask,
		vis->red_mask, vis->green_mask, vis->blue_mask);
	if (img->width < 1 || img->height < 1)
		return NULL;
	if ((size_t)img->width > (size_t)-1 / 3 / (size_t)img->height)
		return NULL;
	out = malloc((size_t)img->width * img->height * 3);
	if (!out)
		return NULL;
	for (y = 0; y < (unsigned int)img->height; y++) {
		dst = out + (size_t)y * img->width * 3;
		for (x = 0; x < (unsigned int)img->width; x++) {
			shot_rgb_pixel(XGetPixel(img, (int)x, (int)y), img->red_mask,
				img->green_mask, img->blue_mask, dst);
			dst += 3;
		}
	}
	return out;
}

/* Reads pixels, encodes, saves and owns the clipboard. Returns 1 on
 * success; 0 means the caller should try another source. */
static int
shot_capture_drawable(Drawable d, int x, int y, int w, int h)
{
	XImage *img;
	unsigned char *rgb, *png;
	size_t png_len;
	char *plain = NULL, *uri = NULL;

	if (w < 1 || h < 1)
		return 0;
	XSetErrorHandler(xerrordummy);
	img = XGetImage(dpy, d, x, y, (unsigned int)w, (unsigned int)h,
		AllPlanes, ZPixmap);
	XSync(dpy, False);
	XSetErrorHandler(xerror);
	if (!img)
		return 0;
	rgb = shot_rgb_from_image(img);
	XDestroyImage(img);
	if (!rgb) {
		fprintf(stderr, "dwm: shot: out of memory\n");
		return 0;
	}
	png = png_encode_rgb(rgb, (unsigned int)w, (unsigned int)h, &png_len);
	free(rgb);
	if (!png) {
		fprintf(stderr, "dwm: shot: png encoding failed\n");
		return 0;
	}
	plain = shot_save(png, png_len);
	if (plain) {
		size_t n = strlen(plain) + sizeof(shot_uri_prefix) + 2;

		uri = malloc(n);
		if (uri)
			snprintf(uri, n, "%s%s\r\n", shot_uri_prefix, plain);
	}
	shot_clipboard_own(png, png_len, plain, uri);
	return 1;
}

static void
shot_capture(int x, int y, int w, int h)
{
	if (x < 0) { w += x; x = 0; }
	if (y < 0) { h += y; y = 0; }
	if (x + w > sw)
		w = sw - x;
	if (y + h > sh)
		h = sh - y;
	if (w < 1 || h < 1)
		return;
	if (!shot_capture_drawable(root, x, y, w, h))
		fprintf(stderr, "dwm: shot: capture failed\n");
}

/* Fullscreen clients page-flip their own buffers, which can leave the root
 * pixmap stale; read the window directly and fall back to the root region
 * when the driver refuses. */
static void
shot_capture_client(Client *c, Monitor *m)
{
	XWindowAttributes wa;

	if (XGetWindowAttributes(dpy, c->win, &wa) && wa.map_state == IsViewable
			&& wa.width >= 1 && wa.height >= 1
			&& shot_capture_drawable(c->win, 0, 0, wa.width, wa.height))
		return;
	shot_capture(m->mx, m->my, m->mw, m->mh);
}

static void
shot_capture_monitor(void)
{
	int x, y;
	Monitor *m;
	Client *c;

	if (getrootptr(&x, &y))
		m = recttomon(x, y, 1, 1);
	else
		m = selmon;
	if (!m)
		return;
	c = m->sel;
	if (c && c->isfullscreen)
		shot_capture_client(c, m);
	else
		shot_capture(m->mx, m->my, m->mw, m->mh);
}

/* Print: crosshair drag selects an area, a plain click takes the monitor.
 * A game holding the pointer falls back to the full monitor. */
/* Region pixels come from the root, except when a focused fullscreen
 * client owns the area: a page-flipping game can leave the root pixmap
 * stale, so read the window itself for the same coordinates. */
static void
shot_region_capture(int x, int y, int w, int h)
{
	Client *c;
	XWindowAttributes wa;

	c = selmon->sel;
	if (c && c->isfullscreen && XGetWindowAttributes(dpy, c->win, &wa)
			&& wa.map_state == IsViewable
			&& x >= wa.x && y >= wa.y
			&& x + w <= wa.x + 2 * wa.border_width + wa.width
			&& y + h <= wa.y + 2 * wa.border_width + wa.height
			&& shot_capture_drawable(c->win, x - wa.x - wa.border_width,
				y - wa.y - wa.border_width, w, h))
		return;
	shot_capture(x, y, w, h);
}

static void
shot_region(const Arg *arg)
{
	int x, y, w, h, r;

	(void)arg;
	r = shot_select(&x, &y, &w, &h);
	if (x < 0) { w += x; x = 0; }
	if (y < 0) { h += y; y = 0; }
	if (x + w > sw)
		w = sw - x;
	if (y + h > sh)
		h = sh - y;
	if (r == SHOT_REGION) {
		if (!(shot_bg && w > 0 && h > 0
				&& shot_capture_drawable(shot_bg, x, y, w, h)))
			shot_region_capture(x, y, w, h);
	} else if (r == SHOT_MON || r == SHOT_GRABFAIL) {
		Monitor *m = getrootptr(&x, &y) ? recttomon(x, y, 1, 1) : selmon;

		if (!(shot_bg && m && shot_capture_drawable(shot_bg, m->mx, m->my,
				m->mw, m->mh)))
			shot_capture_monitor();
	}
	shot_freeze_destroy();
}

/* Shift+Print: whole monitor under the pointer, no grabs needed. */
static void
shot_monitor(const Arg *arg)
{
	(void)arg;
	if (!shot_busy)
		shot_capture_monitor();
}

/* Super+Shift+Print: every monitor at once. */
static void
shot_screen(const Arg *arg)
{
	(void)arg;
	if (!shot_busy)
		shot_capture(0, 0, sw, sh);
}

/* Modifier state read from the pressed-key map: XQueryKeymap and modifier
 * queries are not affected by grabs, so this works inside games too. */
static unsigned int
shot_key_modifiers(void)
{
	XModifierKeymap *mm;
	char keys[32];
	unsigned int mods = 0;
	int i, j, idx;

	mm = XGetModifierMapping(dpy);
	if (!mm)
		return 0;
	if (XQueryKeymap(dpy, keys)) {
		for (i = 0; i < 8; i++)
			for (j = 0; j < mm->max_keypermod; j++) {
				idx = i * mm->max_keypermod + j;
				if (mm->modifiermap[idx]
						&& (keys[mm->modifiermap[idx] / 8]
							& (1 << (mm->modifiermap[idx] % 8))))
					mods |= 1u << i;
			}
	}
	XFreeModifiermap(mm);
	return mods;
}

/*
 * XI2 raw key hook from the input module. Passive grabs lose to an active
 * keyboard grab held by another client, so a game that calls XGrabKeyboard
 * would otherwise swallow Print entirely. Raw events bypass every grab, so
 * when this path is live the shot keys carry no core grab at all and they
 * fire exactly once per physical press in any context.
 */
void
shot_raw_key(int pressed, KeyCode keycode)
{
	static char down[256];
	KeySym keysym;
	unsigned int mods;
	int i;

	if (keycode < 8 || keycode > 255)
		return;
	if (!pressed) {
		down[keycode] = 0;
		return;
	}
	/* The server suppresses releases while a key repeats, so an edge
	 * trigger fires once per physical press, grabbed or not. */
	if (down[keycode])
		return;
	down[keycode] = 1;
	keysym = XKeycodeToKeysym(dpy, keycode, 0);
	if (keysym != XK_Print)
		return;
	mods = CLEANMASK(shot_key_modifiers());
	for (i = 0; i < LENGTH(keys); i++) {
		if (!keys[i].func || keys[i].keysym != keysym)
			continue;
		if (CLEANMASK(keys[i].mod) != mods)
			continue;
		/* Only shot keys ride raw events; everything else stays a
		 * passive grab so normal binds keep their usual semantics. */
		if (keys[i].func != shot_region && keys[i].func != shot_monitor
				&& keys[i].func != shot_screen)
			continue;
		keys[i].func(&(keys[i].arg));
		return;
	}
}
