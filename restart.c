/* See LICENSE file for copyright and license details.
 *
 * Restarting in place: dwm executes itself again and the new instance picks
 * the windows back up with their tags, monitor, floating state and order,
 * along with each monitor's tags, layouts and master area. The state is
 * handed over in X properties, which outlive the process. dwm restarts on its
 * own whenever its binary is replaced, e.g. by make install.
 * Included from dwm.c.
 */

enum { StateVersion = 1, StateHeader = 4, MonFields = 9, ClientFields = 8 };

static int restarting;
static char exepath[PATH_MAX];
static char exename[NAME_MAX + 1];
static int exefd = -1;
static int exepending;
static struct timespec exedeadline;
static Atom stateatom, clientstateatom;

void
restart(const Arg *arg)
{
	restarting = 1;
	running = 0;
}

static void
initrestart(void)
{
	char dir[PATH_MAX], *slash;
	ssize_t n;

	stateatom = XInternAtom(dpy, "_DWM_RESTART_STATE", False);
	clientstateatom = XInternAtom(dpy, "_DWM_RESTART_CLIENT", False);
	if ((n = readlink("/proc/self/exe", exepath, sizeof(exepath) - 1)) == -1) {
		exepath[0] = '\0';
		return;
	}
	exepath[n] = '\0';
	snprintf(dir, sizeof(dir), "%s", exepath);
	if (!(slash = strrchr(dir, '/')))
		return;
	*slash = '\0';
	snprintf(exename, sizeof(exename), "%s", slash + 1);
	/* installing replaces the file, so watch the directory */
	if ((exefd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC)) == -1
	|| inotify_add_watch(exefd, *dir ? dir : "/", IN_CLOSE_WRITE | IN_MOVED_TO) == -1) {
		fprintf(stderr, "dwm: cannot watch %s, no restart on install: %s\n", exepath, strerror(errno));
		if (exefd != -1)
			close(exefd);
		exefd = -1;
	}
}

/* the new binary has to at least start before we replace ourselves with it */
static int
exeworks(void)
{
	char out[64];
	int fd[2];
	size_t len = 0;
	ssize_t n;
	pid_t pid;

	if (access(exepath, X_OK) || pipe(fd) == -1)
		return 0;
	if ((pid = fork()) == -1) {
		close(fd[0]);
		close(fd[1]);
		return 0;
	}
	if (pid == 0) {
		close(ConnectionNumber(dpy));
		close(fd[0]);
		dup2(fd[1], STDOUT_FILENO);
		dup2(fd[1], STDERR_FILENO);
		execl(exepath, exepath, "-v", (char *)NULL);
		_exit(127);
	}
	close(fd[1]);
	while (len < sizeof(out) - 1 && (n = read(fd[0], out + len, sizeof(out) - 1 - len)) > 0)
		len += n;
	close(fd[0]);
	out[len] = '\0';
	return !strncmp(out, "dwm-", 4);
}

static void
exechanged(void)
{
	union {
		struct inotify_event ev;
		char buf[4096];
	} u;
	struct inotify_event *ev;
	ssize_t n;
	char *p;

	while ((n = read(exefd, u.buf, sizeof(u.buf))) > 0)
		for (p = u.buf; p < u.buf + n; p += sizeof(*ev) + ev->len) {
			ev = (struct inotify_event *)p;
			if (ev->len && !strcmp(ev->name, exename)) {
				/* wait for the install to settle (chmod and such) */
				exepending = 1;
				clock_gettime(CLOCK_MONOTONIC, &exedeadline);
				exedeadline.tv_nsec += 400 * 1000000L;
				exedeadline.tv_sec += exedeadline.tv_nsec / 1000000000L;
				exedeadline.tv_nsec %= 1000000000L;
			}
		}
}

/* how long select() may sleep before a pending restart is due, NULL for ever */
static struct timeval *
exetimeout(struct timeval *tv)
{
	struct timespec now;
	long ms;

	if (!exepending)
		return NULL;
	clock_gettime(CLOCK_MONOTONIC, &now);
	ms = (exedeadline.tv_sec - now.tv_sec) * 1000 + (exedeadline.tv_nsec - now.tv_nsec) / 1000000;
	ms = MAX(ms, 0);
	tv->tv_sec = ms / 1000;
	tv->tv_usec = ms % 1000 * 1000;
	return tv;
}

static void
exetimer(void)
{
	struct timeval tv;

	if (!exepending || exetimeout(&tv)->tv_sec || tv.tv_usec)
		return;
	exepending = 0;
	if (exeworks())
		restart(NULL);
	else
		setcfgerr("new dwm binary does not start, not restarting");
}

static void
savestate(void)
{
	long *data, *md, cd[ClientFields];
	int nmons = 0, order;
	Monitor *m;
	Client *c;

	for (m = mons; m; m = m->next)
		nmons++;
	data = ecalloc(StateHeader + nmons * MonFields, sizeof(long));
	data[0] = StateVersion;
	data[1] = selmon->num;
	data[2] = selmon->sel ? selmon->sel->win : None;
	data[3] = nmons;
	for (m = mons, md = data + StateHeader; m; m = m->next, md += MonFields) {
		md[0] = m->tagset[0];
		md[1] = m->tagset[1];
		md[2] = m->seltags;
		md[3] = m->sellt;
		md[4] = m->lt[0] - layouts;
		md[5] = m->lt[1] - layouts;
		md[6] = m->mfact * 10000;
		md[7] = m->nmaster;
		md[8] = m->showbar;
		for (c = m->clients, order = 0; c; c = c->next, order++) {
			/* fullscreen clients keep what they return to */
			cd[0] = c->tags;
			cd[1] = m->num;
			cd[2] = order;
			cd[3] = c->isfullscreen ? c->oldstate : c->isfloating;
			cd[4] = c->isfullscreen ? c->oldx : c->x;
			cd[5] = c->isfullscreen ? c->oldy : c->y;
			cd[6] = c->isfullscreen ? c->oldw : c->w;
			cd[7] = c->isfullscreen ? c->oldh : c->h;
			XChangeProperty(dpy, c->win, clientstateatom, XA_CARDINAL, 32,
				PropModeReplace, (unsigned char *)cd, ClientFields);
		}
	}
	XChangeProperty(dpy, root, stateatom, XA_CARDINAL, 32, PropModeReplace,
		(unsigned char *)data, StateHeader + nmons * MonFields);
	XSync(dpy, False);
	free(data);
}

/* reads and deletes a property left behind by savestate() */
static long *
takestate(Window w, Atom prop, unsigned long minitems)
{
	Atom type;
	int format;
	unsigned long n, after;
	unsigned char *p = NULL;

	if (XGetWindowProperty(dpy, w, prop, 0, 1024, True, XA_CARDINAL,
	    &type, &format, &n, &after, &p) != Success)
		return NULL;
	if (type != XA_CARDINAL || format != 32 || n < minitems) {
		XFree(p);
		return NULL;
	}
	return (long *)p;
}

static Monitor *
numtomon(long num)
{
	Monitor *m;

	for (m = mons; m && m->num != num; m = m->next);
	return m;
}

typedef struct {
	Client *c;
	long order;
} Restored;

static int
cmporder(const void *a, const void *b)
{
	long x = ((const Restored *)a)->order, y = ((const Restored *)b)->order;

	return (x > y) - (x < y);
}

static void
restorestate(void)
{
	long *data, *md, *cd;
	size_t i, n = 0;
	Restored *rs;
	Monitor *m, *tm;
	Client *c;
	Window selwin;
	XEvent ev;

	if (!(data = takestate(root, stateatom, StateHeader)))
		return;
	if (data[0] != StateVersion) {
		XFree(data);
		return;
	}
	for (i = 0, md = data + StateHeader; i < (size_t)data[3]; i++, md += MonFields) {
		if (!(m = numtomon(i)))
			break;
		m->tagset[0] = md[0] & TAGMASK ? md[0] & TAGMASK : 1;
		m->tagset[1] = md[1] & TAGMASK ? md[1] & TAGMASK : 1;
		m->seltags = md[2] & 1;
		m->sellt = md[3] & 1;
		if (md[4] >= 0 && md[4] < (long)LENGTH(layouts))
			m->lt[0] = &layouts[md[4]];
		if (md[5] >= 0 && md[5] < (long)LENGTH(layouts))
			m->lt[1] = &layouts[md[5]];
		m->mfact = MAX(0.05, MIN(md[6] / 10000.0, 0.95));
		m->nmaster = MAX(md[7], 0);
		if (m->showbar != !!md[8]) {
			m->showbar = !!md[8];
			updatebarpos(m);
			XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, m->ww, bh);
		}
	}

	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			n++;
	rs = ecalloc(MAX(n, 1), sizeof(Restored));
	n = 0;
	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			rs[n++].c = c;
	for (i = 0; i < n; i++) {
		c = rs[i].c;
		rs[i].order = -1; /* windows mapped since the restart go first */
		if (!(cd = takestate(c->win, clientstateatom, ClientFields)))
			continue;
		if (cd[0] & TAGMASK)
			c->tags = cd[0] & TAGMASK;
		if ((tm = numtomon(cd[1])))
			c->mon = tm;
		rs[i].order = cd[2];
		if (c->isfullscreen) {
			c->oldstate = cd[3];
			c->oldx = (int)cd[4];
			c->oldy = (int)cd[5];
			c->oldw = (int)cd[6];
			c->oldh = (int)cd[7];
			resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
		} else if ((c->isfloating = cd[3])) {
			resizeclient(c, (int)cd[4], (int)cd[5], (int)cd[6], (int)cd[7]);
		}
		XFree(cd);
	}
	/* rebuild the client lists in their old order */
	qsort(rs, n, sizeof(Restored), cmporder);
	for (m = mons; m; m = m->next)
		m->clients = m->stack = m->sel = NULL;
	for (i = n; i-- > 0;) {
		attach(rs[i].c);
		attachstack(rs[i].c);
	}
	free(rs);

	if ((tm = numtomon(data[1])))
		selmon = tm;
	selwin = data[2];
	XFree(data);
	c = wintoclient(selwin);
	focus(c && ISVISIBLE(c) ? c : NULL);
	arrange(NULL);
	/* windows popping up under the pointer must not steal focus */
	XSync(dpy, False);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

static void
restartcleanup(void)
{
	if (exefd != -1)
		close(exefd);
	exefd = -1;
}
