static int
reload_state_save(void)
{
	ReloadHeader header = { 0x44574d52U, 3, 0, 0, selmon->num };
	ReloadMonitor monitor;
	ReloadClient client;
	Monitor *m;
	Client *c, *s;
	FILE *file;
	unsigned int i;
	int fd = -1;

	for (m = mons; m; m = m->next) {
		header.monitors++;
		for (c = m->clients; c; c = c->next) header.clients++;
	}
	if (!(file = tmpfile())) return -1;
	if (fwrite(&header, sizeof header, 1, file) != 1) goto out;
	for (m = mons; m; m = m->next) {
		memset(&monitor, 0, sizeof monitor);
		monitor.num = m->num;
		monitor.nmaster = m->nmaster;
		monitor.showbar = m->showbar;
		monitor.topbar = m->topbar;
		monitor.seltags = m->seltags;
		monitor.sellt = m->sellt;
		monitor.mfact = m->mfact;
		monitor.selected = m->sel ? m->sel->win : None;
		memcpy(monitor.tagset, m->tagset, sizeof monitor.tagset);
		for (i = 0; i < LENGTH(layouts); i++) {
			if (m->lt[0] == &layouts[i]) monitor.layout[0] = i;
			if (m->lt[1] == &layouts[i]) monitor.layout[1] = i;
		}
		if (fwrite(&monitor, sizeof monitor, 1, file) != 1) goto out;
	}
	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next) {
			memset(&client, 0, sizeof client);
			client.window = c->win;
			client.monitor = m->num;
			client.x = c->x; client.y = c->y; client.w = c->w; client.h = c->h;
			client.oldx = c->oldx; client.oldy = c->oldy;
			client.oldw = c->oldw; client.oldh = c->oldh;
			client.floating = c->isfloating;
			client.fullscreen = c->isfullscreen;
			client.oldstate = c->oldstate;
			client.tags = c->tags;
			for (s = m->stack; s && s != c; s = s->snext) client.stackorder++;
			if (fwrite(&client, sizeof client, 1, file) != 1) goto out;
		}
	/* V2 appends weights after the unchanged V1 client records. */
	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			if (fwrite(&c->cfact, sizeof c->cfact, 1, file) != 1) goto out;
	if (!bsp_checkpoint_write(file)) goto out;
	if (fflush(file) || fseek(file, 0, SEEK_SET)) goto out;
	/* dup deliberately survives exec; fclose removes the original handle. */
	fd = dup(fileno(file));
out:
	fclose(file);
	return fd;
}
