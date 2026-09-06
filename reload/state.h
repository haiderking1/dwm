#ifndef DWM_RELOAD_STATE_H
#define DWM_RELOAD_STATE_H

/* Private, same-machine checkpoint passed through an unlinked file. */
typedef struct {
	unsigned int magic, version, monitors, clients;
	int selected_monitor;
} ReloadHeader;
typedef struct {
	int num, nmaster, showbar, topbar;
	unsigned int tagset[2], seltags, sellt, layout[2];
	float mfact;
	Window selected;
} ReloadMonitor;
typedef struct {
	Window window;
	int monitor, x, y, w, h, oldx, oldy, oldw, oldh;
	int floating, fullscreen, oldstate;
	unsigned int tags, stackorder;
} ReloadClient;

static int reload_state_save(void);
static int reload_state_restore(int fd);

#endif
