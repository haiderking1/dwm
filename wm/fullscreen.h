/* Uses dwm client state to restore layout and geometry on exit. */
#ifndef DWM_FULLSCREEN_H
#define DWM_FULLSCREEN_H

static void
togglefullscreen(const Arg *arg)
{
	(void)arg;
	if (selmon && selmon->sel)
		setfullscreen(selmon->sel, !selmon->sel->isfullscreen);
}

#endif
