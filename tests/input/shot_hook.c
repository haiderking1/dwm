/* Recording stand-in for the shot module raw key hook. */
#include "fake_server.h"

#include <X11/Xlib.h>

int shot_hook_pressed = -1;
KeyCode shot_hook_keycode = 0;

void
shot_raw_key(int pressed, KeyCode keycode)
{
	shot_hook_pressed = pressed;
	shot_hook_keycode = keycode;
}
