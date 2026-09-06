/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = { "dmenu_run", "-m", dmenumon, "-fn", dmenufont, "-nb", col_gray1, "-nf", col_gray3, "-sb", col_cyan, "-sf", col_gray4, NULL };
static const char *termcmd[]  = { "/usr/bin/alacritty", NULL };

static const char *filecmd[] = { "/usr/bin/nautilus", NULL };
static const char *browsercmd[] = { "/usr/bin/zen-browser", NULL };
static const char *roficmd[] = { "/usr/bin/rofi", "-show", "drun", NULL };
