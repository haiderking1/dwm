static void
reload_message(const char *message)
{
	snprintf(stext, sizeof stext, "dwm: %s", message);
	drawbars();
}

static void
reloadwm(const Arg *arg)
{
	int result;
	(void)arg;
	result = reload_build_start(reload_source, reload_prefix, reload_log, ConnectionNumber(dpy));
	if (result > 0) reload_message("rebuilding...");
	else if (result < 0) reload_message("cannot start rebuild; see session log");
}

static void
reload_apply(void)
{
	int checkpoint, flags, saved_errno;
	char descriptor[32];

	checkpoint = reload_state_save();
	if (checkpoint < 0) {
		reload_message("reload checkpoint failed; still running");
		return;
	}
	flags = fcntl(ConnectionNumber(dpy), F_GETFD);
	if (flags < 0 || fcntl(ConnectionNumber(dpy), F_SETFD, flags | FD_CLOEXEC) < 0) {
		close(checkpoint);
		reload_message("cannot prepare reload; still running");
		return;
	}
	snprintf(descriptor, sizeof descriptor, "%d", checkpoint);
	XSync(dpy, False);
	execl(reload_binary, "dwm", "--reload-fd", descriptor, (char *)NULL);
	saved_errno = errno;
	fcntl(ConnectionNumber(dpy), F_SETFD, flags);
	close(checkpoint);
	fprintf(stderr, "dwm: reload exec: %s\n", strerror(saved_errno));
	reload_message("reload exec failed; still running");
}

static int
reload_parse_fd(const char *value)
{
	char *end;
	long fd;
	struct stat info;
	errno = 0;
	fd = strtol(value, &end, 10);
	if (errno || !*value || *end || fd < 3 || fd > INT_MAX ||
	    fstat((int)fd, &info) < 0 || !S_ISREG(info.st_mode) ||
	    info.st_uid != geteuid() || info.st_size < (off_t)sizeof(ReloadHeader) ||
	    fcntl((int)fd, F_SETFD, FD_CLOEXEC) < 0)
		die("dwm: invalid reload checkpoint");
	return (int)fd;
}
