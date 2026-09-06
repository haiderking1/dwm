#include "test.h"

void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
static long budget = -1;
void test_alloc_fail(long count) { budget = count; }
static int fail(void)
{
    if (budget < 0)
        return 0;
    if (!budget)
        return 1;
    --budget;
    return 0;
}
void *__wrap_malloc(size_t size)
{
    return fail() ? NULL : __real_malloc(size);
}
void *__wrap_calloc(size_t count, size_t size)
{
    return fail() ? NULL : __real_calloc(count, size);
}
static void count_leaf(uint64_t id, BspRect rect, void *context)
{
    size_t *count = context;
    (void)id; (void)rect;
    ++*count;
}

void test_oom(void)
{
    BspForest forest = {0}, source = {0};
    BspTree *tree;
    BspWorkspace *old;
    BspNode *survivor;
    BspRect bounds = {0, 0, 1200, 800};
    uint64_t initial[] = {1, 2}, next[] = {2, 3, 4, 5}, generation;
    unsigned char *bytes;
    size_t count, length;
    long attempt;
    int result, successes = 0;
    FILE *stream;
    test_alloc_fail(0);
    CHECK(!bsp_workspace(&forest, 0, 1, 1));
    test_alloc_fail(-1);
    CHECK(!forest.views);
    for (attempt = 0; attempt < 10; ++attempt) {
        tree = test_view(&forest, 0, 1);
        CHECK(bsp_tree_sync(tree, initial, 2, 0, bounds));
        survivor = bsp_find(tree, 2);
        test_alloc_fail(attempt);
        result = bsp_tree_sync(tree, next, 4, 2, bounds);
        test_alloc_fail(-1);
        CHECK(bsp_find(tree, 2) == survivor);
        count = 0;
        bsp_tree_visit(tree, count_leaf, &count);
        test_tree(tree, count);
        if (result) {
            CHECK(count == 4);
            ++successes;
        }
        bsp_forest_clear(&forest);
    }
    CHECK(successes > 0 && successes < 10);
    CHECK(bsp_tree_sync(test_view(&source, 0, 1), next, 4, 0, bounds));
    bytes = test_bytes(&source, &length);
    successes = 0;
    for (attempt = 0; attempt < 13; ++attempt) {
        tree = test_view(&forest, 8, 2);
        CHECK(bsp_tree_sync(tree, initial, 2, 0, bounds));
        old = forest.views;
        generation = tree->generation;
        test_alloc_fail(attempt);
        result = test_read_bytes(bytes, length, &forest);
        test_alloc_fail(-1);
        if (result) {
            ++successes;
            test_equal(&source.views->tree, &forest.views->tree);
        } else {
            CHECK(forest.views == old && tree->generation == generation);
            test_tree(tree, 2);
        }
        bsp_forest_clear(&forest);
    }
    CHECK(successes > 0 && successes < 13);
    stream = tmpfile();
    CHECK(stream);
    test_alloc_fail(0);
    result = bsp_forest_write(stream, &source);
    test_alloc_fail(-1);
    CHECK(!result && ftell(stream) == 0 && fclose(stream) == 0);
    free(bytes);
    bsp_forest_clear(&source);
}
