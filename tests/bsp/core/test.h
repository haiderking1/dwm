#ifndef BSP_CORE_TEST_H
#define BSP_CORE_TEST_H
#include "../../../wm/bsp/bsp.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(expr) do { if (!(expr)) test_fail(#expr, __FILE__, __LINE__); } while (0)
void test_fail(const char *, const char *, int);
void test_tree(const BspTree *, size_t);
void test_equal(const BspTree *, const BspTree *);
BspTree *test_view(BspForest *, int, uint32_t);
void test_membership(void);
void test_operations(void);
void test_layout_resize(void);
void test_geometry(void);
void test_corruption(void);
void test_persistence(void);
void test_random(void);
void test_oom(void);
void test_limits(void);
void test_alloc_fail(long);
unsigned char *test_bytes(const BspForest *, size_t *);
int test_read_bytes(const unsigned char *, size_t, BspForest *);
void test_put32(unsigned char *, uint32_t);
void test_put64(unsigned char *, uint64_t);
#endif
