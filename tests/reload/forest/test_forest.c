#include <math.h>
#include <stddef.h>
#include "fixture.h"
#include "roundtrip.h"
#include "rejection.h"
#include "corruption.h"
#include "migration.h"

int
main(void)
{
	test_forest_roundtrip();
	test_empty_forest_roundtrip();
	test_forest_truncation();
	test_forest_trailing_data();
	test_forest_corruption();
	test_wrapper_mask_validation();
	test_invalid_prefix_with_valid_forest();
	test_legacy_forest_migration(1);
	test_legacy_forest_migration(2);
	bsp_forest_clear(&checkpoint_forest);
	puts("reload BSP forest tests passed");
	return 0;
}
