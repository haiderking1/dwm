/*
 * In-process CLIPBOARD owner. dwm itself answers selection requests, so the
 * screenshot stays valid as long as the X server does. There is no helper
 * process for a game to kill or race with, which is why this replaces the
 * xclip dance. Large images switch to the ICCCM INCR protocol and are
 * chunked as requestors delete each property window.
 */

static Atom shot_a_clipboard, shot_a_targets, shot_a_timestamp, shot_a_incr,
	shot_a_png, shot_a_uri, shot_a_utf8, shot_a_text, shot_a_tsprop;
static int shot_atoms_ready;
static Window shot_selwin;
static Time shot_sel_time;
static unsigned char *shot_png;
static size_t shot_png_len;
static char *shot_text_plain, *shot_text_uri;

#define SHOT_XFERS 8
struct shot_xfer {
	Window requestor;
	Atom property;
	size_t offset;
	int active;
};
static struct shot_xfer shot_xfers[SHOT_XFERS];

static void
shot_atoms(void)
{
	static const char *names[] = { "CLIPBOARD", "TARGETS", "TIMESTAMP", "INCR",
		"image/png", "text/uri-list", "UTF8_STRING", "text/plain",
		"_DWM_SHOT_TIME" };
	Atom atoms[LENGTH(names)];

	if (shot_atoms_ready)
		return;
	if (!XInternAtoms(dpy, (char **)names, (int)LENGTH(names), False, atoms))
		return;
	shot_a_clipboard = atoms[0];
	shot_a_targets = atoms[1];
	shot_a_timestamp = atoms[2];
	shot_a_incr = atoms[3];
	shot_a_png = atoms[4];
	shot_a_uri = atoms[5];
	shot_a_utf8 = atoms[6];
	shot_a_text = atoms[7];
	shot_a_tsprop = atoms[8];
	shot_atoms_ready = 1;
}

static Window
shot_owner_window(void)
{
	if (!shot_selwin) {
		XSetWindowAttributes wa = { .override_redirect = True };

		shot_selwin = XCreateWindow(dpy, root, -2, -2, 1, 1, 0,
			CopyFromParent, InputOutput, CopyFromParent,
			CWOverrideRedirect, &wa);
	}
	return shot_selwin;
}

static Bool
shot_ts_match(Display *display, XEvent *event, XPointer arg)
{
	(void)display;
	(void)arg;
	return event->type == PropertyNotify && event->xproperty.atom == shot_a_tsprop;
}

/* Ask the server for a real timestamp through a property round trip. */
static Time
shot_server_time(void)
{
	Window win = shot_owner_window();
	XEvent ev;

	XSelectInput(dpy, win, PropertyChangeMask);
	XDeleteProperty(dpy, win, shot_a_tsprop);
	XChangeProperty(dpy, win, shot_a_tsprop, XA_CARDINAL, 8, PropModeReplace,
		(unsigned char *)"", 1);
	XIfEvent(dpy, &ev, shot_ts_match, NULL);
	XDeleteProperty(dpy, win, shot_a_tsprop);
	return ev.xproperty.time;
}

static struct shot_xfer *
shot_xfer_start(Window requestor, Atom property)
{
	int i;

	for (i = 0; i < SHOT_XFERS; i++) {
		if (!shot_xfers[i].active) {
			shot_xfers[i].active = 1;
			shot_xfers[i].requestor = requestor;
			shot_xfers[i].property = property;
			shot_xfers[i].offset = 0;
			return &shot_xfers[i];
		}
	}
	return NULL;
}

static void
shot_xfer_stop(struct shot_xfer *x, int deselect)
{
	x->active = 0;
	if (deselect)
		XSelectInput(dpy, x->requestor, NoEventMask);
}

static void
shot_xfers_stop_all(void)
{
	int i;

	for (i = 0; i < SHOT_XFERS; i++)
		if (shot_xfers[i].active)
			shot_xfer_stop(&shot_xfers[i], 1);
}

static size_t
shot_chunk_size(void)
{
	long units = XMaxRequestSize(dpy);
	size_t bytes = units > 0 ? (size_t)units * 4 : 262144;

	return bytes > 8192 ? bytes - 4096 : 8192;
}

/* Send the next INCR chunk; a zero-length chunk ends the transfer. */
static void
shot_xfer_pump(struct shot_xfer *x)
{
	size_t remain, chunk;
	XWindowAttributes wa;

	if (!XGetWindowAttributes(dpy, x->requestor, &wa)) {
		shot_xfer_stop(x, 0); /* requestor died mid-transfer */
		return;
	}
	remain = shot_png_len - x->offset;
	chunk = remain < shot_chunk_size() ? remain : shot_chunk_size();
	XChangeProperty(dpy, x->requestor, x->property, shot_a_png, 8, PropModeReplace,
		shot_png ? shot_png + x->offset : NULL, (int)chunk);
	if (chunk == 0) {
		shot_xfer_stop(x, 1);
		return;
	}
	x->offset += chunk;
}

static void
shot_notify(Window requestor, Atom selection, Atom target, Atom property, Time time)
{
	XSelectionEvent reply;

	reply.type = SelectionNotify;
	reply.display = dpy;
	reply.requestor = requestor;
	reply.selection = selection;
	reply.target = target;
	reply.property = property;
	reply.time = time;
	XSendEvent(dpy, requestor, False, NoEventMask, (XEvent *)&reply);
}

static void
shot_send_targets(XSelectionRequestEvent *ev)
{
	Atom targets[8];
	Atom prop = ev->property != None ? ev->property : ev->target;
	int n = 0;

	targets[n++] = shot_a_targets;
	targets[n++] = shot_a_timestamp;
	if (shot_png)
		targets[n++] = shot_a_png;
	if (shot_text_plain) {
		targets[n++] = shot_a_text;
		targets[n++] = shot_a_utf8;
		targets[n++] = XA_STRING;
	}
	if (shot_text_uri)
		targets[n++] = shot_a_uri;
	XChangeProperty(dpy, ev->requestor, prop, XA_ATOM, 32, PropModeReplace,
		(unsigned char *)targets, n);
	shot_notify(ev->requestor, ev->selection, ev->target, prop, ev->time);
}

static void
shot_selrequest(XEvent *e)
{
	XSelectionRequestEvent *ev = &e->xselectionrequest;
	unsigned char *data = NULL;
	size_t len = 0;
	Atom type = None, prop;

	shot_atoms();
	if (ev->selection != shot_a_clipboard) {
		shot_notify(ev->requestor, ev->selection, ev->target, None, ev->time);
		return;
	}
	if (ev->target == shot_a_targets) {
		shot_send_targets(ev);
		return;
	}
	prop = ev->property != None ? ev->property : ev->target;

	if (ev->target == shot_a_timestamp) {
		unsigned long t = (unsigned long)shot_sel_time;

		XChangeProperty(dpy, ev->requestor, prop, XA_INTEGER, 32,
			PropModeReplace, (unsigned char *)&t, 1);
		shot_notify(ev->requestor, ev->selection, ev->target, prop, ev->time);
		return;
	}

	if (ev->target == shot_a_png && shot_png) {
		data = shot_png;
		len = shot_png_len;
		type = shot_a_png;
	} else if (shot_text_plain && (ev->target == shot_a_text
			|| ev->target == shot_a_utf8 || ev->target == XA_STRING)) {
		data = (unsigned char *)shot_text_plain;
		len = strlen(shot_text_plain);
		type = ev->target;
	} else if (ev->target == shot_a_uri && shot_text_uri) {
		data = (unsigned char *)shot_text_uri;
		len = strlen(shot_text_uri);
		type = shot_a_uri;
	}

	if (!data || !type) {
		/* Refusal: SelectionNotify with no property. */
		shot_notify(ev->requestor, ev->selection, ev->target, None, ev->time);
		return;
	}

	if (ev->target == shot_a_png && len > shot_chunk_size()) {
		struct shot_xfer *x = shot_xfer_start(ev->requestor, prop);
		unsigned long total = (unsigned long)len;

		if (x) {
			XSelectInput(dpy, ev->requestor,
				PropertyChangeMask|StructureNotifyMask);
			XChangeProperty(dpy, ev->requestor, prop, shot_a_incr, 32,
				PropModeReplace, (unsigned char *)&total, 1);
			shot_notify(ev->requestor, ev->selection, ev->target, prop, ev->time);
			return;
		}
		/* No transfer slot left: refuse rather than truncate. */
		shot_notify(ev->requestor, ev->selection, ev->target, None, ev->time);
		return;
	}

	XChangeProperty(dpy, ev->requestor, prop, type, 8, PropModeReplace,
		data, (int)len);
	shot_notify(ev->requestor, ev->selection, ev->target, prop, ev->time);
}

static void
shot_selclear(XEvent *e)
{
	XSelectionClearEvent *ev = &e->xselectionclear;

	shot_atoms();
	if (ev->selection != shot_a_clipboard || ev->window != shot_selwin)
		return;
	shot_xfers_stop_all();
	free(shot_png);
	free(shot_text_plain);
	free(shot_text_uri);
	shot_png = NULL;
	shot_png_len = 0;
	shot_text_plain = NULL;
	shot_text_uri = NULL;
}

static int
shot_propertynotify(XPropertyEvent *ev)
{
	int i;

	if (!shot_atoms_ready)
		return 0;
	for (i = 0; i < SHOT_XFERS; i++) {
		if (shot_xfers[i].active && ev->window == shot_xfers[i].requestor
				&& ev->atom == shot_xfers[i].property
				&& ev->state == PropertyDelete) {
			shot_xfer_pump(&shot_xfers[i]);
			return 1;
		}
	}
	return 0;
}

static void
shot_destroynotify(XDestroyWindowEvent *ev)
{
	int i;

	if (!shot_atoms_ready)
		return;
	for (i = 0; i < SHOT_XFERS; i++)
		if (shot_xfers[i].active && ev->window == shot_xfers[i].requestor)
			shot_xfer_stop(&shot_xfers[i], 0);
}

/*
 * Take ownership of CLIPBOARD for a fresh screenshot. Takes ownership of all
 * four buffers; they are freed when ownership is lost or replaced. Failing
 * ownership frees everything, the saved file remains.
 */
static void
shot_clipboard_own(unsigned char *png, size_t png_len, char *plain, char *uri)
{
	shot_atoms();
	shot_xfers_stop_all();
	free(shot_png);
	free(shot_text_plain);
	free(shot_text_uri);
	shot_png = png;
	shot_png_len = png_len;
	shot_text_plain = plain;
	shot_text_uri = uri;
	shot_sel_time = shot_server_time();
	XSetSelectionOwner(dpy, shot_a_clipboard, shot_owner_window(), shot_sel_time);
	if (XGetSelectionOwner(dpy, shot_a_clipboard) != shot_owner_window()) {
		fprintf(stderr, "dwm: shot: could not own CLIPBOARD\n");
		free(shot_png);
		free(shot_text_plain);
		free(shot_text_uri);
		shot_png = NULL;
		shot_png_len = 0;
		shot_text_plain = NULL;
		shot_text_uri = NULL;
	}
}
