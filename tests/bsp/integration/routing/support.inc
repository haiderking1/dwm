/* Spies cover dispatch only; weighted geometry has its own production-include
 * suite. Any accidental fallthrough to floating/warp/migration aborts. */
static int routed_weighted, routed_pop;
static const int snap = 32;
static void resize_tiled(Client *c) { assert(c == selmon->sel); ++routed_weighted; }
static void restack(Monitor *m) { (void)m; assert(!"unexpected floating resize"); }
static void resize(Client *c, int x, int y, int w, int h, int interact)
{ (void)c; (void)x; (void)y; (void)w; (void)h; (void)interact; assert(0); }
static void togglefloating(const Arg *arg) { (void)arg; assert(0); }
static void sendmon(Client *c, Monitor *m) { (void)c; (void)m; assert(0); }
static Client *nexttiled(Client *c)
{
	for (; c && (c->isfloating || !ISVISIBLE(c)); c = c->next);
	return c;
}
static void pop(Client *c) { assert(c); ++routed_pop; }
int
XWarpPointer(Display *display, Window source, Window destination,
             int sx, int sy, unsigned int sw, unsigned int sh, int dx, int dy)
{
	(void)display; (void)source; (void)destination; (void)sx; (void)sy;
	(void)sw; (void)sh; (void)dx; (void)dy;
	assert(!"tiled resize warped the pointer");
	return 1;
}

/* dwm intentionally leaves some key-binding Arg parameters unused. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
/* setmfact has a local f; only this fixture has a global with that name. */
#pragma GCC diagnostic ignored "-Wshadow"
#include "routes.inc"
#pragma GCC diagnostic pop
