#include "../../input/settings.h"

#include <X11/Xlib.h>

/* The real link must satisfy the shot raw key hook too. */
void shot_raw_key(int pressed, KeyCode keycode);

void
shot_raw_key(int pressed, KeyCode keycode)
{
	(void)pressed;
	(void)keycode;
}

int
main(void)
{
	/* Verify the real Xlib/libXi link without opening or changing a display. */
	input_setup(NULL, None);
	input_handle_event(NULL, NULL);
	return 0;
}
