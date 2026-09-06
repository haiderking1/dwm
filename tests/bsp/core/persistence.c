#include "test.h"
#include <unistd.h>

void test_malformed(const unsigned char *, size_t);

void test_persistence(void)
{
    BspForest forest = {0}, restored = {0}, second = {0};
    BspTree *tree, *other;
    BspWorkspace *view, *copy;
    BspResize resize;
    BspRect bounds = {11, 27, 1349, 791};
    uint64_t ids[] = {42, UINT64_MAX, 91, 12};
    unsigned char *bytes, *again;
    size_t length, again_length;
    FILE *stream;
    int fds[2];
    tree = test_view(&forest, 0, 1);
    other = test_view(&forest, INT_MAX, UINT32_MAX);
    test_view(&forest, 0, 3);
    CHECK(bsp_tree_sync(tree, ids, 4, 0, bounds));
    CHECK(bsp_tree_focus(tree, 91));
    CHECK(bsp_tree_adjust(tree, 42, BSP_HORIZONTAL, 0.13));
    CHECK(bsp_tree_rotate(tree, 12));
    CHECK(bsp_tree_sync(other, ids, 2, 0, bounds));
    bsp_tree_layout(other, bounds, 47);
    bytes = test_bytes(&forest, &length);
    CHECK(test_read_bytes(bytes, length, &restored));
    for (view = forest.views; view; view = view->next) {
        copy = bsp_workspace(&restored, view->monitor, view->tags, 0);
        CHECK(copy);
        test_equal(&view->tree, &copy->tree);
        CHECK(view->tree.generation != copy->tree.generation);
        CHECK(copy->tree.bounds.width == 0 && copy->tree.bounds.height == 0);
        if (copy->tree.root)
            CHECK(copy->tree.root->rect.x == 0 && copy->tree.root->rect.width == 0);
        bsp_tree_layout(&copy->tree, bounds, copy->tree.minimum);
    }
    again = test_bytes(&restored, &again_length);
    CHECK(length == again_length && !memcmp(bytes, again, length));
    free(again);
    /* Exactly one frame is consumed, without seeking or reading to EOF. */
    CHECK(length * 2 < 4096 && pipe(fds) == 0);
    CHECK(write(fds[1], bytes, length) == (ssize_t)length);
    CHECK(write(fds[1], bytes, length) == (ssize_t)length);
    CHECK(close(fds[1]) == 0);
    stream = fdopen(fds[0], "rb");
    CHECK(stream && bsp_forest_read(stream, &second));
    CHECK(bsp_forest_read(stream, &second));
    CHECK(fgetc(stream) == EOF && fclose(stream) == 0);
    test_equal(tree, &bsp_workspace(&second, 0, 1, 0)->tree);
    CHECK(bsp_resize_begin(tree, 42, 0, 0, &resize));
    CHECK(test_read_bytes(bytes, length, &forest));
    tree = &bsp_workspace(&forest, 0, 1, 0)->tree;
    bsp_tree_layout(tree, bounds, tree->minimum);
    CHECK(!bsp_resize_update(tree, &resize, 10, 10));
    test_malformed(bytes, length);
    free(bytes);
    bsp_forest_clear(&forest);
    bsp_forest_clear(&restored);
    bsp_forest_clear(&second);
    bytes = test_bytes(&forest, &length);
    CHECK(length == 24 && test_read_bytes(bytes, length, &restored));
    free(bytes);
    CHECK(!bsp_forest_read(NULL, &restored));
    CHECK(!bsp_forest_write(NULL, &restored));
}
