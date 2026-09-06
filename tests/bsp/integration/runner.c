#include "support/fixture.h"
#include "../../../wm/bsp/integration/forest.inc"
#include "../../../wm/bsp/integration/actions.inc"
#include "../../../wm/bsp/integration/checkpoint.inc"
#include "support/clients.inc"
#include "support/xlib.inc"
#include "support/events.inc"
#include "../../../wm/bsp/integration/mouse.inc"
#include "../../../wm/drag/placement.inc"
#include "../../../wm/drag/bsp.inc"
#include "../../../wm/drag/mouse.inc"
#include "cases/layout.inc"
#include "cases/checkpoint.inc"
#include "cases/drag.inc"
#include "cases/mouse.inc"
#include "routing/support.inc"
#include "routing/cases.inc"

int
main(void)
{
	test_layout(); puts("bsp integration: membership, focus, views, fullscreen");
	test_actions(); puts("bsp integration: split rotation, horizontal ratio, zoom");
	test_checkpoint(); puts("bsp integration: checkpoint validation and ownership");
	test_drag(); puts("bsp integration: same and mixed-monitor drag swaps");
	test_mouse_coordinates(); puts("bsp integration: resize release coordinates and focus");
	test_mouse_cancellation(); puts("bsp integration: resize lifecycle cancellation");
	test_routing(); puts("bsp integration: actual dwm routing and configuration indices");
	reset(); free(mons); mons = selmon = NULL;
	return 0;
}
