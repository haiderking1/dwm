/* See LICENSE file for copyright and license details.
 *
 * config.lua: keybindings and input settings, loaded from
 * $XDG_CONFIG_HOME/dwm/config.lua and reloaded whenever the file is saved.
 * A config that fails to load leaves the running one untouched and shows the
 * error in the bar. Without any usable config.lua the keys from config.h
 * apply. Included from dwm.c.
 */

static lua_State *L;            /* owns the functions bound in keys */
static Key *keys;
static size_t nkeys;
static Key *newkeys;            /* collected while config.lua runs */
static size_t nnewkeys;
static InputConf newinputconf;
static int loading;
static int reloadpending;
static char cfgpath[PATH_MAX + sizeof("/config.lua")];
static char cfgdir[PATH_MAX];
static char cfgerr[256];
static int inotifyfd = -1;
static int cfgwatch[2] = { -1, -1 };
static char watchname[2][NAME_MAX + 1];

static int l_spawn(lua_State *s);

static void
setcfgerr(const char *msg)
{
	size_t n = strlen(cfgdir);

	if (!msg) {
		cfgerr[0] = '\0';
	} else {
		fprintf(stderr, "dwm: %s\n", msg);
		/* the full path does not fit in the bar */
		if (n && !strncmp(msg, cfgdir, n) && msg[n] == '/')
			msg += n + 1;
		snprintf(cfgerr, sizeof(cfgerr), "%s", msg);
	}
	if (selmon)
		drawbars();
}

static void
luacall(const Arg *arg)
{
	int fn;

	lua_rawgeti(L, LUA_REGISTRYINDEX, arg->i);
	if (!(fn = lua_isfunction(L, -1))) {
		/* a command string or argv table, spawn it */
		lua_pushcfunction(L, l_spawn);
		lua_insert(L, -2);
	}
	if (lua_pcall(L, fn ? 0 : 1, 0, 0) != LUA_OK) {
		setcfgerr(lua_tostring(L, -1));
		lua_pop(L, 1);
	}
}

static unsigned int
parsemod(lua_State *s, const char *name)
{
	static const struct { const char *name; unsigned int mask; } mods[] = {
		{ "super", Mod4Mask }, { "mod4", Mod4Mask }, { "win", Mod4Mask },
		{ "alt", Mod1Mask }, { "mod1", Mod1Mask },
		{ "shift", ShiftMask },
		{ "ctrl", ControlMask }, { "control", ControlMask },
		{ "mod2", Mod2Mask }, { "mod3", Mod3Mask }, { "mod5", Mod5Mask },
	};
	size_t i;

	for (i = 0; i < LENGTH(mods); i++)
		if (!strcasecmp(name, mods[i].name))
			return mods[i].mask;
	return luaL_error(s, "unknown modifier '%s'", name);
}

/* "super+shift" or { "super", "shift" }, "" for none */
static unsigned int
parsemods(lua_State *s, int idx)
{
	char buf[128], *tok, *p;
	unsigned int mask = 0;
	lua_Integer i, n;

	if (lua_istable(s, idx)) {
		n = luaL_len(s, idx);
		for (i = 1; i <= n; i++) {
			lua_rawgeti(s, idx, i);
			mask |= parsemod(s, luaL_checkstring(s, -1));
			lua_pop(s, 1);
		}
		return mask;
	}
	snprintf(buf, sizeof(buf), "%s", luaL_checkstring(s, idx));
	for (p = buf; (tok = strtok(p, "+ ")); p = NULL)
		mask |= parsemod(s, tok);
	return mask;
}

/* dwm.key(mods, key, action), action: function, command string or argv table */
static int
l_key(lua_State *s)
{
	unsigned int mod;
	KeySym sym, lower, upper;
	Key *k;

	if (!loading)
		return luaL_error(s, "dwm.key can only be called while config.lua loads");
	mod = parsemods(s, 1);
	if ((sym = XStringToKeysym(luaL_checkstring(s, 2))) == NoSymbol)
		return luaL_error(s, "unknown key '%s'", lua_tostring(s, 2));
	XConvertCase(sym, &lower, &upper);
	if (!lua_isfunction(s, 3) && !lua_isstring(s, 3) && !lua_istable(s, 3))
		return luaL_argerror(s, 3, "expected function, command string or argv table");
	if (!(k = realloc(newkeys, (nnewkeys + 1) * sizeof(Key))))
		return luaL_error(s, "out of memory");
	newkeys = k;
	k = &newkeys[nnewkeys++];
	k->mod = mod;
	k->keysym = lower;
	k->func = luacall;
	lua_pushvalue(s, 3);
	k->arg.i = luaL_ref(s, LUA_REGISTRYINDEX);
	return 0;
}

/* dwm.input{ mouse_accel = false, mouse_speed = 0, repeat_delay = 200, repeat_rate = 35 } */
static int
l_input(lua_State *s)
{
	InputConf ic = loading ? newinputconf : inputconf;
	const char *field;

	luaL_checktype(s, 1, LUA_TTABLE);
	lua_pushnil(s);
	while (lua_next(s, 1)) {
		field = lua_type(s, -2) == LUA_TSTRING ? lua_tostring(s, -2) : "";
		if (!strcmp(field, "mouse_accel")) {
			luaL_checktype(s, -1, LUA_TBOOLEAN);
			ic.mouseaccel = lua_toboolean(s, -1);
		} else if (!strcmp(field, "mouse_speed")) {
			ic.mousespeed = luaL_checknumber(s, -1);
			if (ic.mousespeed < -1 || ic.mousespeed > 1)
				return luaL_error(s, "mouse_speed must be between -1 and 1");
		} else if (!strcmp(field, "repeat_delay")) {
			ic.repeatdelay = luaL_checkinteger(s, -1);
		} else if (!strcmp(field, "repeat_rate")) {
			ic.repeatrate = luaL_checkinteger(s, -1);
		} else {
			return luaL_error(s, "dwm.input: unknown setting '%s'", field);
		}
		lua_pop(s, 1);
	}
	if (loading) {
		newinputconf = ic;
	} else {
		inputconf = ic;
		applyinput();
	}
	return 0;
}

/* dwm.spawn("cmd args") runs through /bin/sh, dwm.spawn{ "cmd", "arg" } does not */
static int
l_spawn(lua_State *s)
{
	const char **argv;
	lua_Integer i, n;
	Arg a;

	if (lua_istable(s, 1)) {
		if ((n = luaL_len(s, 1)) < 1)
			return luaL_argerror(s, 1, "empty argv table");
		luaL_checkstack(s, n, "argv table too long");
		argv = lua_newuserdatauv(s, (n + 1) * sizeof(*argv), 0);
		for (i = 0; i < n; i++) {
			lua_rawgeti(s, 1, i + 1);
			if (!lua_isstring(s, -1))
				return luaL_error(s, "dwm.spawn: argv[%d] is not a string", (int)i + 1);
			argv[i] = lua_tostring(s, -1); /* anchored on the stack */
		}
		argv[n] = NULL;
	} else {
		argv = lua_newuserdatauv(s, 4 * sizeof(*argv), 0);
		argv[0] = "/bin/sh";
		argv[1] = "-c";
		argv[2] = luaL_checkstring(s, 1);
		argv[3] = NULL;
	}
	a.v = argv;
	spawn(&a);
	return 0;
}

/* tag number 1..ntags, 0 for all tags, nil for none */
static unsigned int
tagarg(lua_State *s)
{
	lua_Integer t;

	if (lua_isnoneornil(s, 1))
		return 0;
	t = luaL_checkinteger(s, 1);
	if (t < 0 || t > (lua_Integer)LENGTH(tags))
		return luaL_error(s, "no tag %d, there are %d", (int)t, (int)LENGTH(tags));
	return t ? 1 << (t - 1) : ~0;
}

static int
l_setlayout(lua_State *s)
{
	Arg a = { .v = NULL }; /* nil toggles back to the previous layout */
	lua_Integer i;

	if (!lua_isnoneornil(s, 1)) {
		i = luaL_checkinteger(s, 1);
		if (i < 1 || i > (lua_Integer)LENGTH(layouts))
			return luaL_error(s, "no layout %d, there are %d", (int)i, (int)LENGTH(layouts));
		a.v = &layouts[i - 1];
	}
	setlayout(&a);
	return 0;
}

static int
l_reload(lua_State *s)
{
	if (!loading)
		reloadpending = 1;
	return 0;
}

#define LUA_NOARG(f)  static int l_##f(lua_State *s) { f(&(Arg){0}); return 0; }
#define LUA_INTARG(f) static int l_##f(lua_State *s) { f(&(Arg){.i = luaL_checkinteger(s, 1)}); return 0; }
#define LUA_TAGARG(f) static int l_##f(lua_State *s) { f(&(Arg){.ui = tagarg(s)}); return 0; }
LUA_NOARG(killclient)
LUA_NOARG(quit)
LUA_NOARG(togglebar)
LUA_NOARG(togglefloating)
LUA_NOARG(togglefullscreen)
LUA_NOARG(zoom)
LUA_INTARG(focusmon)
LUA_INTARG(focusstack)
LUA_INTARG(incnmaster)
LUA_INTARG(tagmon)
LUA_TAGARG(tag)
LUA_TAGARG(toggletag)
LUA_TAGARG(toggleview)
LUA_TAGARG(view)

static int
l_setmfact(lua_State *s)
{
	setmfact(&(Arg){.f = luaL_checknumber(s, 1)});
	return 0;
}

static const luaL_Reg luaapi[] = {
	{ "key",              l_key },
	{ "input",            l_input },
	{ "spawn",            l_spawn },
	{ "reload",           l_reload },
	{ "quit",             l_quit },
	{ "killclient",       l_killclient },
	{ "togglebar",        l_togglebar },
	{ "togglefloating",   l_togglefloating },
	{ "togglefullscreen", l_togglefullscreen },
	{ "zoom",             l_zoom },
	{ "focusmon",         l_focusmon },
	{ "focusstack",       l_focusstack },
	{ "incnmaster",       l_incnmaster },
	{ "setmfact",         l_setmfact },
	{ "setlayout",        l_setlayout },
	{ "tagmon",           l_tagmon },
	{ "tag",              l_tag },
	{ "toggletag",        l_toggletag },
	{ "toggleview",       l_toggleview },
	{ "view",             l_view },
	{ NULL, NULL }
};

static int
loadconfig(void)
{
	lua_State *s;

	if (!(s = luaL_newstate())) {
		setcfgerr("cannot create Lua state");
		return 0;
	}
	luaL_openlibs(s);
	lua_newtable(s);
	luaL_setfuncs(s, luaapi, 0);
	lua_pushinteger(s, LENGTH(tags));
	lua_setfield(s, -2, "ntags");
	lua_setglobal(s, "dwm");
	/* let config.lua require() files next to it */
	lua_getglobal(s, "package");
	lua_pushfstring(s, "%s/?.lua;%s/?/init.lua;", cfgdir, cfgdir);
	lua_getfield(s, -2, "path");
	lua_concat(s, 2);
	lua_setfield(s, -2, "path");
	lua_pop(s, 1);

	newkeys = NULL;
	nnewkeys = 0;
	newinputconf = definputconf(); /* settings left out of config.lua reset */
	loading = 1;
	if (luaL_dofile(s, cfgpath) != LUA_OK) {
		loading = 0;
		setcfgerr(lua_tostring(s, -1));
		lua_close(s);
		free(newkeys);
		newkeys = NULL;
		return 0;
	}
	loading = 0;

	free(keys);
	keys = newkeys;
	nkeys = nnewkeys;
	newkeys = NULL;
	if (L)
		lua_close(L);
	L = s;
	inputconf = newinputconf;
	applyinput();
	grabkeys();
	setcfgerr(NULL);
	return 1;
}

static void
watchconfig(void)
{
	char real[PATH_MAX], *slash;
	int wd = -1;

	if (inotifyfd == -1)
		return;
	/* watch directories, editors often replace the file instead of writing it */
	if (cfgwatch[0] == -1) {
		if ((cfgwatch[0] = inotify_add_watch(inotifyfd, cfgdir, IN_CLOSE_WRITE | IN_MOVED_TO)) == -1)
			fprintf(stderr, "dwm: cannot watch %s: %s\n", cfgdir, strerror(errno));
		snprintf(watchname[0], sizeof(watchname[0]), "%s", strrchr(cfgpath, '/') + 1);
	}
	/* and the directory a symlinked config.lua points into. The new watch is
	 * added before the old one goes, so no write slips through in between */
	if (realpath(cfgpath, real) && (slash = strrchr(real, '/'))) {
		*slash = '\0';
		wd = inotify_add_watch(inotifyfd, real, IN_CLOSE_WRITE | IN_MOVED_TO);
		snprintf(watchname[1], sizeof(watchname[1]), "%s", slash + 1);
	}
	if (cfgwatch[1] != -1 && cfgwatch[1] != wd && cfgwatch[1] != cfgwatch[0])
		inotify_rm_watch(inotifyfd, cfgwatch[1]);
	cfgwatch[1] = wd;
}

static void
reloadconfig(void)
{
	loadconfig();
	watchconfig(); /* the symlink target may have changed */
}

static void
configchanged(void)
{
	union {
		struct inotify_event ev;
		char buf[4096];
	} u;
	struct inotify_event *ev;
	ssize_t n;
	char *p;
	int i, changed = 0;

	while ((n = read(inotifyfd, u.buf, sizeof(u.buf))) > 0)
		for (p = u.buf; p < u.buf + n; p += sizeof(*ev) + ev->len) {
			ev = (struct inotify_event *)p;
			for (i = 0; i < LENGTH(cfgwatch); i++)
				if (ev->wd == cfgwatch[i] && ev->len && !strcmp(ev->name, watchname[i]))
					changed = 1;
		}
	if (changed)
		reloadconfig();
}

static void
luainit(void)
{
	const char *base, *home;

	initinput();
	if ((base = getenv("XDG_CONFIG_HOME")) && *base)
		snprintf(cfgdir, sizeof(cfgdir), "%s/dwm", base);
	else if ((home = getenv("HOME")))
		snprintf(cfgdir, sizeof(cfgdir), "%s/.config/dwm", home);
	snprintf(cfgpath, sizeof(cfgpath), "%s/config.lua", cfgdir);

	if ((inotifyfd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC)) == -1)
		fprintf(stderr, "dwm: inotify: %s, config.lua will not hot reload\n", strerror(errno));
	watchconfig();
	if (!access(cfgpath, F_OK) && loadconfig())
		return;
	/* no usable config.lua, fall back to config.h */
	keys = ecalloc(LENGTH(defkeys), sizeof(Key));
	memcpy(keys, defkeys, sizeof(defkeys));
	nkeys = LENGTH(defkeys);
	applyinput();
	grabkeys();
}

static void
luacleanup(void)
{
	if (L)
		lua_close(L);
	L = NULL;
	free(keys);
	keys = NULL;
	nkeys = 0;
	if (inotifyfd != -1)
		close(inotifyfd);
	inotifyfd = -1;
}
