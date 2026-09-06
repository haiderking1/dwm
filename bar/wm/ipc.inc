#include "json.h"

static struct {
	Atom state, command, utf8;
	char *published;
} qs_ipc;

static void
qs_ipc_setup(void)
{
	qs_ipc.state = XInternAtom(dpy, "_DWM_QUICKSHELL_STATE", False);
	qs_ipc.command = XInternAtom(dpy, "_DWM_QUICKSHELL_COMMAND", False);
	qs_ipc.utf8 = XInternAtom(dpy, "UTF8_STRING", False);
}

/* Called once after each bounded event batch, and after scan/restore. */
static void
qs_publish(void)
{
	char *json = NULL;
	size_t length = 0, i;
	FILE *out;
	Monitor *m;
	Client *c;
	unsigned int occupied, urgent;
	if (!(out = open_memstream(&json, &length))) die("dwm: state stream:");
	fputs("{\"tags\":[", out);
	for (i = 0; i < LENGTH(tags); i++) {
		if (i) fputc(',', out);
		qs_json_string(out, tags[i]);
	}
	fprintf(out, "],\"selectedMonitor\":%d,\"monitors\":[", selmon ? selmon->num : -1);
	for (m = mons; m; m = m->next) {
		occupied = urgent = 0;
		for (c = m->clients; c; c = c->next) {
			occupied |= c->tags;
			if (c->isurgent) urgent |= c->tags;
		}
		if (m != mons) fputc(',', out);
		fprintf(out, "{\"num\":%d,\"x\":%d,\"y\":%d,\"width\":%d,\"height\":%d,"
		             "\"selected\":%u,\"occupied\":%u,\"urgent\":%u,\"layout\":",
		        m->num, m->mx, m->my, m->mw, m->mh,
		        m->tagset[m->seltags] & TAGMASK, occupied & TAGMASK, urgent & TAGMASK);
		qs_json_string(out, m->ltsymbol);
		fputs(",\"title\":", out);
		qs_json_string(out, m->sel && ISVISIBLE(m->sel) ? m->sel->name : "");
		fprintf(out, ",\"fullscreen\":%s}",
		        m->sel && ISVISIBLE(m->sel) && m->sel->isfullscreen ? "true" : "false");
	}
	fputs("]}", out);
	if (ferror(out)) die("dwm: write state:");
	if (fclose(out)) die("dwm: close state:");
	if (!qs_ipc.published || strcmp(qs_ipc.published, json)) {
		if (length > INT_MAX) die("dwm: state too large");
		XChangeProperty(dpy, root, qs_ipc.state, qs_ipc.utf8, 8,
		                PropModeReplace, (unsigned char *)json, (int)length);
		free(qs_ipc.published);
		qs_ipc.published = json;
		XFlush(dpy);
	} else free(json);
}

static int
qs_command(XClientMessageEvent *event)
{
	Monitor *m;
	Arg arg = {0};
	unsigned int i;
	long op = event->data.l[0];
	if (event->message_type != qs_ipc.command) return 0;
	if (event->format != 32 || op < 1 || op > 4) return 1;
	for (m = mons; m && m->num != event->data.l[1]; m = m->next);
	if (!m) return 1;
	if (op == 1 || op == 2) {
		arg.ui = (unsigned long)event->data.l[2] & TAGMASK;
		if (!arg.ui) return 1;
	}
	/* An external-mode toggle is deliberately inert, including monitor focus. */
	if (op == 4 && externalbar) return 1;
	if (m != selmon) {
		unfocus(selmon->sel, 0);
		selmon = m;
		focus(m->sel);
	}
	switch (op) {
	case 1: view(&arg); break;
	case 2: tag(&arg); break;
	case 3:
		for (i = 0; i < LENGTH(layouts) && &layouts[i] != m->lt[m->sellt]; i++);
		arg.v = &layouts[(i + 1) % LENGTH(layouts)];
		setlayout(&arg);
		break;
	case 4: togglebar(&arg); break;
	}
	return 1;
}

static void
qs_ipc_cleanup(void)
{
	XDeleteProperty(dpy, root, qs_ipc.state);
	free(qs_ipc.published);
	qs_ipc.published = NULL;
}
