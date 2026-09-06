#ifndef DWM_BSP_HOOKS_H
#define DWM_BSP_HOOKS_H
/* Included after dwm's Client/Monitor declarations and bsp.h. */
static void dwindle(Monitor *);
static void bsp_focus_client(Client *);
static void bsp_resize_mouse(Client *);
static void bsp_rotate_selected(const Arg *);
static int bsp_checkpoint_write(FILE *);
static int bsp_checkpoint_read(FILE *, BspForest *);
static void bsp_checkpoint_apply(BspForest *);
#endif
