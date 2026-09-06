#include "test.h"
int main(void)
{
    test_membership();
    test_operations();
    test_layout_resize();
    test_geometry();
    test_persistence();
    test_corruption();
    test_limits();
    test_random();
    test_oom();
    puts("BSP core: membership, operations, layout/resize, persistence, limits, random, OOM passed");
    return 0;
}
