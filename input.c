/* See LICENSE file for copyright and license details.
 *
 * Input device settings: key repeat and pointer acceleration. They are
 * applied at startup, on every config reload and whenever a keyboard or
 * mouse is plugged in, since the X server resets them for new devices.
 * Included from dwm.c.
 */

typedef struct {
	int mouseaccel;
	float mousespeed;
	int repeatdelay;
	int repeatrate;
} InputConf;

static InputConf inputconf;
static int xiopcode = -1;

static InputConf
definputconf(void)
{
	InputConf ic = { mouseaccel, mousespeed, repeatdelay, repeatrate };
	return ic;
}

static int
hasprop(int dev, Atom prop, Atom *type, int *format, unsigned long *n)
{
	Atom t;
	int f;
	unsigned long nitems, after;
	unsigned char *data = NULL;

	if (prop == None || XIGetProperty(dpy, dev, prop, 0, 16, False, AnyPropertyType,
	    &t, &f, &nitems, &after, &data) != Success)
		return 0;
	XFree(data);
	if (type)
		*type = t;
	if (format)
		*format = f;
	if (n)
		*n = nitems;
	return t != None;
}

static void
setpointeraccel(int dev)
{
	Atom profile, speed, tapping, type;
	unsigned char flags[3] = { 0, 0, 0 }; /* adaptive, flat, custom */
	unsigned long n;
	int format;
	float f;

	profile = XInternAtom(dpy, "libinput Accel Profile Enabled", True);
	speed = XInternAtom(dpy, "libinput Accel Speed", True);
	tapping = XInternAtom(dpy, "libinput Tapping Enabled", True);

	/* leave touchpads alone, they want acceleration */
	if (hasprop(dev, tapping, NULL, NULL, NULL))
		return;
	if (hasprop(dev, profile, &type, &format, &n) && format == 8 && n >= 2) {
		flags[inputconf.mouseaccel ? 0 : 1] = 1;
		XIChangeProperty(dpy, dev, profile, type, 8, PropModeReplace,
			flags, MIN(n, LENGTH(flags)));
	}
	if (hasprop(dev, speed, &type, &format, NULL) && format == 32) {
		f = inputconf.mousespeed;
		XIChangeProperty(dpy, dev, speed, type, 32, PropModeReplace,
			(unsigned char *)&f, 1);
	}
}

static void
applyinput(void)
{
	XIDeviceInfo *devs;
	int i, n, delay, interval;

	/* devices can vanish under us, errors are expected here */
	XSync(dpy, False);
	XSetErrorHandler(xerrordummy);

	delay = inputconf.repeatdelay;
	interval = inputconf.repeatrate > 0 ? 1000 / inputconf.repeatrate : 0;
	if (delay > 0 && interval > 0) {
		XAutoRepeatOn(dpy);
		XkbSetAutoRepeatRate(dpy, XkbUseCoreKbd, delay, interval);
	}
	/* core acceleration, only matters for non-libinput devices */
	if (inputconf.mouseaccel)
		XChangePointerControl(dpy, True, True, -1, -1, -1);
	else
		XChangePointerControl(dpy, True, True, 1, 1, 0);

	if (xiopcode != -1 && (devs = XIQueryDevice(dpy, XIAllDevices, &n))) {
		for (i = 0; i < n; i++) {
			if (devs[i].use == XISlaveKeyboard && delay > 0 && interval > 0)
				XkbSetAutoRepeatRate(dpy, devs[i].deviceid, delay, interval);
			else if (devs[i].use == XISlavePointer)
				setpointeraccel(devs[i].deviceid);
		}
		XIFreeDeviceInfo(devs);
	}

	XSync(dpy, False);
	XSetErrorHandler(xerror);
}

void
genericevent(XEvent *e)
{
	XGenericEventCookie *cookie = &e->xcookie;
	XIHierarchyEvent *ev;

	if (cookie->extension != xiopcode || !XGetEventData(dpy, cookie))
		return;
	if (cookie->evtype == XI_HierarchyChanged) {
		ev = cookie->data;
		if (ev->flags & (XISlaveAdded | XIDeviceEnabled))
			applyinput();
	}
	XFreeEventData(dpy, cookie);
}

static void
initinput(void)
{
	int event, error, major = 2, minor = 0;
	unsigned char bits[XIMaskLen(XI_LASTEVENT)] = { 0 };
	XIEventMask mask;

	inputconf = definputconf();
	if (!XQueryExtension(dpy, "XInputExtension", &xiopcode, &event, &error)
	|| XIQueryVersion(dpy, &major, &minor) != Success) {
		fputs("dwm: XInput 2 not available, only core input settings apply\n", stderr);
		xiopcode = -1;
		return;
	}
	XISetMask(bits, XI_HierarchyChanged);
	mask.deviceid = XIAllDevices;
	mask.mask_len = sizeof(bits);
	mask.mask = bits;
	XISelectEvents(dpy, root, &mask, 1);
}
