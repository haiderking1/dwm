#include "../../input/settings.h"

int
main(void)
{
	/* Verify the real Xlib/libXi link without opening or changing a display. */
	input_setup(NULL, None);
	input_handle_event(NULL, NULL);
	return 0;
}
