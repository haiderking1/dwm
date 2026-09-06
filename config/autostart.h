/* Apps launched once each time dwm starts, after existing windows are scanned.
 * Uncomment or add entries below. Each argument is a separate quoted string.
 * End each command and the list with NULL. No shell expansion or & is needed.
 * Apps run independently; starting dwm again launches this list again.
 * Rebuild: make -C ~/dwm install PREFIX="$HOME/.local"
 */
static const char *const *const autostart[] = {
	(const char *const[]){ "waypaper", "--restore", NULL },
	(const char *const[]){ "quickshell", "--path", "/home/soka/.config/quickshell/dwm", "--no-duplicate", NULL },
	/* (const char *const[]){ "alacritty", NULL }, */
	/* (const char *const[]){ "nautilus", "--new-window", NULL }, */
	/* (const char *const[]){ "zen-browser", NULL }, */
	NULL
};
