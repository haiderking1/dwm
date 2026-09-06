#include "fake_server.h"

#include <stdio.h>

int
main(void)
{
	test_properties();
	test_events();
	puts("input settings tests passed");
	return 0;
}
