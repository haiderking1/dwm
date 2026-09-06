#ifndef BRIDGE_WALLPAPER_H
#define BRIDGE_WALLPAPER_H
#include <X11/Xlib.h>
int bridge_wallpaper_emit(Display *display, Window root);
void bridge_wallpaper_cleanup(void);
#endif
