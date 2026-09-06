#include "support/fixture.h"
#include "../../wm/resize/math.h"
#include "../../wm/resize/layout.inc"
#include "../../wm/resize/topology.inc"
#include "../../wm/resize/precision.inc"
#include "../../wm/resize/boundary.inc"
#include "../../wm/resize/mouse.inc"
#include "../../wm/drag/placement.inc"
#include "support/clients.inc"
#include "support/events.inc"
#include "support/xlib.inc"
#include "cases/allocation.inc"
#include "cases/boundaries.inc"
#include "cases/precision.inc"
#include "cases/locked.inc"
#include "mouse/coordinates.inc"
#include "mouse/lifecycle.inc"

int
main(void)
{
	test_allocation();
	test_weight_bounds();
	test_pairwise_boundaries();
	test_boundary_choice();
	test_weighted_swap();
	test_precision_siblings();
	test_locked_boundaries();
	test_mouse_coordinates();
	test_mouse_buttons();
	test_mouse_failures();
	test_mouse_lifecycle();
	puts("resize: allocation, boundaries, weighted slots and mocked mouse events passed");
	return 0;
}
