#include "support/fixture.h"

/* Test the production implementation, not a copy or a stub. */
#include "../../wm/drag/placement.inc"

#include "support/swap_oracle.inc"
#include "cases/target.inc"
#include "cases/same_monitor.inc"
#include "cases/cross_monitor.inc"
#include "cases/rejections.inc"

int
main(void)
{
	test_target_geometry();
	test_target_stack();
	test_same_monitor_swaps();
	test_cross_monitor_swaps();
	test_invalid_swaps();
	test_missing_membership();
	puts("drag placement: 6 test groups passed");
	return 0;
}
