#include "test.h"

static void reject(const unsigned char *bytes, size_t length, BspForest *out)
{
    BspWorkspace *view = out->views;
    BspNode *root = view->tree.root;
    uint64_t generation = view->tree.generation;
    CHECK(!test_read_bytes(bytes, length, out));
    CHECK(out->views == view && view->tree.root == root);
    CHECK(view->tree.generation == generation && root->window == 777);
    test_tree(&view->tree, 1);
}

void test_malformed(const unsigned char *original, size_t length)
{
    BspForest out = {0}, sample = {0}, empty = {0};
    BspTree *tree = test_view(&out, 9, 9);
    uint64_t id = 777, ids[] = {1, 2};
    BspRect bounds = {0, 0, 1000, 600};
    unsigned char *bytes, *base;
    size_t i, n;
    static const struct { size_t offset; uint32_t value; } mutations[] = {
        {8, 2}, {12, 1025}, {16, 131072}, {20, 1},
        {24, UINT32_MAX}, {28, 0}, {32, 2}, {32, 8193},
        {36, 0}, {36, UINT32_MAX}, {40, 9}, {48, 0}, {48, 1},
        {56, 0}, {64, 4}, {80, 2}, {80, 0x100}, {80, 0x10000},
        {88, 0}, {96, 0}, {112, 0x200}, {112, 0x10001}
    };
    CHECK(bsp_tree_sync(tree, &id, 1, 0, bounds));
    for (i = 0; i < length; ++i)
        reject(original, i, &out);
    CHECK(bsp_tree_sync(test_view(&sample, 0, 1), ids, 2, 0, bounds));
    base = test_bytes(&sample, &n);
    CHECK(n == 152);
    bytes = malloc(n);
    CHECK(bytes);
    for (i = 0; i < sizeof mutations / sizeof mutations[0]; ++i) {
        memcpy(bytes, base, n);
        test_put32(bytes + mutations[i].offset, mutations[i].value);
        reject(bytes, n, &out);
    }
    memcpy(bytes, base, n);
    bytes[0] ^= 0xff;
    reject(bytes, n, &out);
    /* Duplicate window, duplicate node identity, NaN, infinity, and low ratio. */
    memcpy(bytes, base, n);
    memcpy(bytes + 136 - 8, bytes + 96, 8);
    reject(bytes, n, &out);
    memcpy(bytes, base, n);
    memcpy(bytes + 120, bytes + 88, 8);
    reject(bytes, n, &out);
    memcpy(bytes, base, n);
    test_put64(bytes + 72, UINT64_C(0x7ff8000000000000));
    reject(bytes, n, &out);
    test_put64(bytes + 72, UINT64_C(0x7ff0000000000000));
    reject(bytes, n, &out);
    test_put64(bytes + 72, 0);
    reject(bytes, n, &out);
    test_put64(bytes + 72, UINT64_C(0x3ff0000000000000));
    reject(bytes, n, &out);
    /* Root claims to be a leaf but its declared child records remain. */
    memcpy(bytes, base, n);
    test_put64(bytes + 64, 1);
    bytes[81] = 1;
    reject(bytes, n, &out);
    /* A branch requiring children exhausts the explicit node budget. */
    memcpy(bytes, base, n);
    test_put32(bytes + 16, 1);
    test_put32(bytes + 32, 1);
    reject(bytes, n, &out);
    CHECK(!test_read_bytes(bytes, n, &empty) && !empty.views);
    free(bytes);
    free(base);
    bsp_forest_clear(&sample);
    test_view(&sample, 0, 1);
    test_view(&sample, 0, 2);
    base = test_bytes(&sample, &n);
    CHECK(n == 88);
    memcpy(base + 56, base + 24, 8);
    reject(base, n, &out);
    free(base);
    bsp_forest_clear(&sample);
    bsp_forest_clear(&out);
}
