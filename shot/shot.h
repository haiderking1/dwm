/* See LICENSE file for copyright and license details. */

#ifndef SHOT_SHOT_H
#define SHOT_SHOT_H

#include <X11/Xlib.h>

/* XI2 raw key hook called by the input module. Raw events reach dwm even
 * while another client holds a keyboard grab, which is how the Print keys
 * keep working inside games. Implemented in shot/shot.inc inside dwm.c. */
void shot_raw_key(int pressed, KeyCode keycode);

#endif
