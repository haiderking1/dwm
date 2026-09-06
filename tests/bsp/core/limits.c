#include "test.h"

static void record(unsigned char *p, uint64_t id, uint64_t window)
{
    test_put64(p, id);
    test_put64(p + 8, window);
    test_put64(p + 16, UINT64_C(0x3fe0000000000000));
    p[25] = (unsigned char)(window != 0);
}

static unsigned char *frame(unsigned nodes, size_t *length)
{
    unsigned char *bytes;
    *length = 56 + (size_t)nodes * 32;
    bytes = calloc(1, *length);
    CHECK(bytes);
    memcpy(bytes, "BSPFRST", 7);
    test_put32(bytes + 8, 1);
    test_put32(bytes + 12, 1);
    test_put32(bytes + 16, nodes);
    test_put32(bytes + 28, 1);
    test_put32(bytes + 32, nodes);
    test_put32(bytes + 36, 32);
    test_put64(bytes + 40, 1);
    test_put64(bytes + 48, nodes + 1);
    return bytes;
}

static void balanced(unsigned char *bytes, unsigned leaves,
                      unsigned *node, unsigned *window)
{
    unsigned index = (*node)++;
    if (leaves == 1) {
        record(bytes + 56 + index * 32, index + 1, (*window)++);
        return;
    }
    record(bytes + 56 + index * 32, index + 1, 0);
    bytes[56 + index * 32 + 24] = (unsigned char)(leaves % 3 == 1);
    balanced(bytes, leaves / 2, node, window);
    balanced(bytes, leaves - leaves / 2, node, window);
}

void test_limits(void)
{
    BspForest forest = {0};
    BspTree *tree;
    BspRect bounds = {0, 0, 1200, 800};
    unsigned char *bytes;
    uint64_t ids[4097], generation;
    unsigned depth, i, node = 0, window = 1;
    size_t length;
    for (depth = 128; depth <= 129; ++depth) {
        bytes = frame(depth * 2 + 1, &length);
        for (i = 0; i < depth; ++i)
            record(bytes + 56 + i * 32, i + 1, 0);
        for (; i < depth * 2 + 1; ++i)
            record(bytes + 56 + i * 32, i + 1, i - depth + 1);
        if (depth == 128) {
            CHECK(test_read_bytes(bytes, length, &forest));
            tree = &forest.views->tree;
            bsp_tree_layout(tree, bounds, 32);
            test_tree(tree, 129);
        } else {
            generation = forest.views->tree.generation;
            CHECK(!test_read_bytes(bytes, length, &forest));
            CHECK(forest.views->tree.generation == generation);
        }
        free(bytes);
    }
    bsp_forest_clear(&forest);
    bytes = frame(8191, &length);
    balanced(bytes, 4096, &node, &window);
    CHECK(node == 8191 && window == 4097);
    CHECK(test_read_bytes(bytes, length, &forest));
    free(bytes);
    tree = &forest.views->tree;
    bsp_tree_layout(tree, bounds, 32);
    test_tree(tree, 4096);
    for (i = 0; i < 4097; ++i)
        ids[i] = i + 1;
    generation = tree->generation;
    CHECK(bsp_tree_sync(tree, ids, 4096, 0, bounds));
    CHECK(tree->generation == generation);
    CHECK(!bsp_tree_sync(tree, ids, 4097, 0, bounds));
    bytes = test_bytes(&forest, &length);
    CHECK(test_read_bytes(bytes, length, &forest));
    free(bytes);
    bsp_forest_clear(&forest);
    tree = test_view(&forest, 0, 1);
    CHECK(!bsp_tree_sync(tree, ids, 130, 1, bounds));
    test_tree(tree, 129);
    CHECK(bsp_tree_sync(tree, ids, 2, 0, bounds));
    /* Allocation IDs cannot wrap or be reused after a deletion. */
    tree->next_id = UINT64_MAX;
    generation = tree->generation;
    CHECK(!bsp_tree_sync(tree, ids, 3, 1, bounds));
    CHECK(tree->generation == generation);
    test_tree(tree, 2);
    bsp_forest_clear(&forest);
    for (i = 0; i < 1024; ++i)
        CHECK(bsp_workspace(&forest, (int)i, 1, 1));
    CHECK(!bsp_workspace(&forest, 1024, 1, 1));
    CHECK(bsp_workspace(&forest, 0, 1, 1));
    bytes = test_bytes(&forest, &length);
    CHECK(test_read_bytes(bytes, length, &forest));
    free(bytes);
    bsp_forest_clear(&forest);
}
