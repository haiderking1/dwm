#include "support/fixture.h"
#include "support/clients.inc"
#include "support/xlib.inc"
#include "support/queue.inc"
#include "support/layout.inc"
#include "support/dispatch.inc"

/* Both the placement decisions and the event loop are production code. */
#include "../../../wm/drag/placement.inc"
#include "../../../wm/drag/mouse.inc"

#include "support/assertions.inc"
#include "cases/drops.inc"
#include "cases/lifecycle.inc"
#include "cases/buttons.inc"
#include "cases/changes.inc"

int
main(void)
{
	static const struct { const char *name; void (*run)(void); } tests[] = {
		{ "preview and occupied drop", test_preview_and_occupied_drop },
		{ "empty, bar and empty-monitor snapback", test_empty_and_bar_snapback },
		{ "final release coordinates", test_release_coordinates },
		{ "short click and threshold", test_short_click_and_threshold },
		{ "cross-monitor occupied drop", test_cross_monitor_drop },
		{ "failed grab", test_failed_grab },
		{ "failed root pointer", test_failed_root_pointer },
		{ "source freed during dispatch", test_source_destroyed },
		{ "target freed before release", test_target_destroyed },
		{ "other buttons do not release", test_other_buttons_do_not_release },
		{ "throttled motion release", test_throttled_motion_release },
		{ "pending configure request", test_pending_configure_request },
		{ "pending monitor geometry", test_pending_monitor_geometry },
		{ "pending dock workarea", test_pending_dock_workarea },
		{ "layout change cancels", test_layout_change_cancels },
		{ "target becomes ineligible", test_target_becomes_ineligible },
	};
	size_t i;
	for (i = 0; i < sizeof(tests) / sizeof(tests[0]); ++i) {
		printf("drag mouse: %s\n", tests[i].name);
		fflush(stdout);
		tests[i].run();
	}
	printf("drag mouse: %zu test groups passed\n", i);
	return 0;
}
