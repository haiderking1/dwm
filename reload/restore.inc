static Monitor *
reload_monitor(int number)
{
	Monitor *m;
	for (m = mons; m; m = m->next)
		if (m->num == number) return m;
	return selmon;
}

static int
reload_state_restore(int fd)
{
	ReloadHeader header;
	BspForest forest = {0};
	ReloadMonitor *monitors = NULL;
	ReloadClient *clients = NULL, *saved;
	float *factors = NULL;
	Client **matched = NULL, *c;
	Monitor *m;
	FILE *file;
	unsigned int i, rank;
	int ok = 0;

	if (!(file = fdopen(fd, "rb"))) { close(fd); goto out; }
	if (fread(&header, sizeof header, 1, file) != 1 ||
	    header.magic != 0x44574d52U || (header.version < 1 || header.version > 3) ||
	    header.monitors == 0 || header.monitors > 64 || header.clients > 65536)
		goto out;
	monitors = calloc(header.monitors, sizeof *monitors);
	clients = calloc(header.clients ? header.clients : 1, sizeof *clients);
	matched = calloc(header.clients ? header.clients : 1, sizeof *matched);
	factors = calloc(header.clients ? header.clients : 1, sizeof *factors);
	if (!monitors || !clients || !matched || !factors) goto out;
	if (fread(monitors, sizeof *monitors, header.monitors, file) != header.monitors ||
	    fread(clients, sizeof *clients, header.clients, file) != header.clients)
		goto out;
	if (header.version >= 2) {
		if (fread(factors, sizeof *factors, header.clients, file) != header.clients)
			goto out;
	} else {
		for (i = 0; i < header.clients; i++) factors[i] = 1.0f;
	}
	/* Parse into temporary ownership. No live fields or lists change on failure. */
	if (header.version == 3 &&
	    (!bsp_checkpoint_read(file, &forest) || fgetc(file) != EOF || ferror(file)))
		goto out;
	/* Validate every weight, including unmatched windows, before live changes.
	 * The positive range test also rejects NaN and infinities. */
	for (i = 0; i < header.clients; i++)
		if (!(factors[i] >= 0.001f && factors[i] <= 1000000.0f)) goto out;
	/* Validate the whole checkpoint before changing live client lists. */
	for (i = 0; i < header.monitors; i++)
		if (monitors[i].seltags > 1 || monitors[i].sellt > 1 ||
		    !(monitors[i].mfact >= 0.05f && monitors[i].mfact <= 0.95f)) goto out;
	for (i = 0; i < header.clients; i++) {
		if (clients[i].w <= 0 || clients[i].h <= 0 ||
		    clients[i].stackorder >= header.clients) goto out;
		matched[i] = wintoclient(clients[i].window);
		for (rank = 0; rank < i; rank++)
			if (clients[rank].window == clients[i].window) goto out;
	}
	selmon = reload_monitor(header.selected_monitor);
	for (i = 0; i < header.monitors; i++) {
		m = reload_monitor(monitors[i].num);
		if (m->num != monitors[i].num) continue;
		m->nmaster = MAX(0, monitors[i].nmaster);
		m->showbar = !!monitors[i].showbar;
		m->topbar = !!monitors[i].topbar;
		m->mfact = monitors[i].mfact;
		m->seltags = monitors[i].seltags;
		m->sellt = monitors[i].sellt;
		for (rank = 0; rank < 2; rank++) {
			m->tagset[rank] = monitors[i].tagset[rank] & TAGMASK;
			if (!m->tagset[rank]) m->tagset[rank] = 1;
			m->lt[rank] = &layouts[monitors[i].layout[rank] < LENGTH(layouts) ? monitors[i].layout[rank] : 0];
		}
		updatebarpos(m);
		XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, m->ww, bh);
	}
	for (i = 0; i < header.clients; i++)
		if ((c = matched[i])) { detach(c); detachstack(c); }
	/* Reverse insertion preserves tiling order; new windows remain managed. */
	for (i = header.clients; i > 0; ) {
		--i;
		if (!(c = matched[i])) continue;
		saved = &clients[i];
		c->mon = reload_monitor(saved->monitor);
		c->cfact = factors[i];
		c->tags = saved->tags & TAGMASK;
		if (!c->tags) c->tags = c->mon->tagset[c->mon->seltags];
		c->isfloating = !!saved->floating;
		c->isfullscreen = !!saved->fullscreen;
		c->oldstate = !!saved->oldstate;
		c->bw = c->isfullscreen ? 0 : borderpx;
		c->oldbw = borderpx;
		if (c->isfullscreen)
			resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
		else
			resizeclient(c, saved->x, saved->y, saved->w, saved->h);
		c->oldx = saved->oldx; c->oldy = saved->oldy;
		c->oldw = saved->oldw; c->oldh = saved->oldh;
		attach(c);
	}
	for (rank = header.clients; rank > 0; ) {
		--rank;
		for (i = 0; i < header.clients; i++)
			if (matched[i] && clients[i].stackorder == rank) attachstack(matched[i]);
	}
	for (i = 0; i < header.monitors; i++) {
		m = reload_monitor(monitors[i].num);
		c = wintoclient(monitors[i].selected);
		if (c && c->mon == m) m->sel = c;
	}
	focus(selmon->sel);
	/* Legacy checkpoints apply an empty forest, discarding pre-restore trees. */
	bsp_checkpoint_apply(&forest);
	arrange(NULL);
	ok = 1;
out:
	bsp_forest_clear(&forest);
	free(factors);
	free(matched);
	free(clients);
	free(monitors);
	if (file) fclose(file);
	return ok;
}
