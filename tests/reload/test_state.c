#include "state_fixture.h"

int main(void)
{
	Monitor a = {0}, b = {0};
	Client one = {0}, two = {0}, three = {0}, newcomer = {0};
	Client *c;
	FILE *bad;
	int fd;
	bsp_forest_clear(&checkpoint_forest);
	a.num = 0; b.num = 1; a.next = &b;
	a.mfact = .65f; b.mfact = .55f;
	a.nmaster = 2; b.nmaster = 1;
	a.tagset[0] = 4; a.tagset[1] = 2;
	b.tagset[0] = b.tagset[1] = 512;
	a.lt[0] = &layouts[2]; a.lt[1] = &layouts[1];
	b.lt[0] = &layouts[0]; b.lt[1] = &layouts[1];
	a.mw = b.mw = 1920; a.mh = b.mh = 1080; b.mx = 1920;
	mons = &a; selmon = &b;
	one.win = 1; two.win = 2; three.win = 3; newcomer.win = 4;
	one.cfact = .25f; two.cfact = 2.5f; three.cfact = 7.0f; newcomer.cfact = 1.0f;
	one.mon = two.mon = &a; three.mon = &b; newcomer.mon = &a;
	one.w = two.w = three.w = newcomer.w = 600;
	one.h = two.h = three.h = newcomer.h = 400;
	one.tags = two.tags = 4; three.tags = 512;
	one.x = 120; one.y = 80; one.isfloating = 1;
	three.isfullscreen = three.isfloating = 1; three.oldstate = 0;
	three.oldx = 2020; three.oldy = 30; three.oldw = 600; three.oldh = 400;
	attach(&two); attach(&one); attach(&three);
	attachstack(&one); attachstack(&two); attachstack(&three);
	a.sel = &two; b.sel = &three;
	fd = reload_state_save(); assert(fd >= 0);
	/* Simulate scan moving everything to the initial workspace/monitor. */
	mons = selmon = &a;
	a.clients = a.stack = a.sel = NULL;
	b.clients = b.stack = b.sel = NULL;
	one.mon = two.mon = three.mon = &a;
	one.tags = two.tags = three.tags = 1;
	one.cfact = two.cfact = three.cfact = 1.0f;
	one.x = -1000; three.oldw = 1920;
	a.mfact = .55f; a.nmaster = 1; a.tagset[0] = 1; a.lt[0] = &layouts[0];
	attach(&one); attachstack(&one); attach(&two); attachstack(&two);
	attach(&three); attachstack(&three); attach(&newcomer); attachstack(&newcomer);
	assert(reload_state_restore(fd));
	assert(selmon == &b && b.sel == &three && a.sel == &two);
	assert(one.cfact == .25f && two.cfact == 2.5f && three.cfact == 7.0f);
	assert(newcomer.cfact == 1.0f);
	assert(a.tagset[0] == 4 && b.tagset[0] == 512 && a.nmaster == 2);
	assert(a.mfact == .65f && a.lt[0] == &layouts[2]);
	assert(a.clients == &one && one.next == &two && two.next == &newcomer);
	assert(a.stack == &two && two.snext == &one && one.snext == &newcomer);
	assert(b.clients == &three && b.stack == &three && three.next == NULL);
	assert(one.x == 120 && one.y == 80 && one.isfloating && one.tags == 4);
	assert(three.mon == &b && three.tags == 512 && three.isfullscreen);
	assert(three.x == 1920 && three.w == 1920 && three.oldw == 600 && three.oldx == 2020);
	/* Corrupt state must not partially detach any windows. */
	c = a.clients;
	bad = tmpfile(); assert(bad);
	assert(fputs("bad state", bad) >= 0); assert(fflush(bad) == 0); rewind(bad);
	fd = dup(fileno(bad)); fclose(bad);
	assert(!reload_state_restore(fd) && a.clients == c);
	bsp_forest_clear(&checkpoint_forest);
	puts("reload state tests passed");
	return 0;
}
