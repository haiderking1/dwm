#include "test.h"

static int rect_equal(BspRect a, BspRect b)
{
    return !memcmp(&a, &b, sizeof a);
}

void test_layout_resize(void)
{
    BspForest forest = {0};
    BspTree *tree = test_view(&forest, 0, 1);
    BspRect bounds = {0, 0, 1000, 600}, protected;
    BspResize resize, stale;
    BspNode *root, *parent;
    uint64_t ids[] = {1, 2, 3, 4};
    int i;
    CHECK(bsp_tree_sync(tree, ids, 1, 0, bounds));
    CHECK(!bsp_resize_begin(tree, 1, 0, 0, &resize));
    CHECK(bsp_tree_sync(tree, ids, 2, 1, bounds));
    CHECK(bsp_tree_sync(tree, ids, 3, 2, bounds));
    CHECK(bsp_tree_sync(tree, ids, 4, 2, bounds));
    root = tree->root;
    parent = bsp_find(tree, 2)->parent;
    CHECK(parent->axis == BSP_HORIZONTAL);
    CHECK(bsp_resize_begin(tree, 2, 740, 290, &resize));
    CHECK(resize.horizontal == parent->id && resize.vertical != 0);
    protected = bsp_find(tree, 3)->rect;
    CHECK(bsp_resize_update(tree, &resize, 790, 290));
    CHECK(root->ratio == 0.5 && parent->ratio == 0.6);
    CHECK(rect_equal(protected, bsp_find(tree, 3)->rect));
    CHECK(bsp_resize_begin(tree, 4, 510, 290, &resize));
    CHECK(resize.horizontal == parent->id);
    CHECK(bsp_resize_begin(tree, 2, 505, 295, &resize));
    CHECK(resize.horizontal == root->id);
    CHECK(bsp_resize_update(tree, &resize, 605, 355));
    CHECK(root->ratio == 0.6);
    CHECK(bsp_find(tree, 2)->rect.x == 600 && bsp_find(tree, 2)->rect.height == 360);
    CHECK(bsp_find(tree, 3)->rect.y == 360);
    protected = bsp_find(tree, 2)->rect;
    CHECK(bsp_resize_update(tree, &resize, 605, 355));
    CHECK(rect_equal(protected, bsp_find(tree, 2)->rect));
    CHECK(bsp_resize_update(tree, &resize, INT_MIN, INT_MAX));
    test_tree(tree, 4);
    CHECK(bsp_resize_begin(tree, 2, 0, 0, &stale));
    CHECK(bsp_tree_swap(tree, 2, 4));
    CHECK(!bsp_resize_update(tree, &stale, 10, 10));
    CHECK(bsp_resize_begin(tree, 2, 0, 0, &stale));
    CHECK(bsp_tree_rotate(tree, 2));
    CHECK(!bsp_resize_update(tree, &stale, 10, 10));
    CHECK(bsp_resize_begin(tree, 2, 0, 0, &stale));
    bounds.x = 10;
    bsp_tree_layout(tree, bounds, 32);
    CHECK(!bsp_resize_update(tree, &stale, 10, 10));
    CHECK(bsp_resize_begin(tree, 2, 0, 0, &stale));
    bsp_tree_layout(tree, bounds, 33);
    CHECK(!bsp_resize_update(tree, &stale, 10, 10));
    CHECK(bsp_resize_begin(tree, 2, 0, 0, &stale));
    CHECK(bsp_tree_focus(tree, 4));
    CHECK(!bsp_resize_update(tree, &stale, 10, 10));
    CHECK(bsp_resize_begin(tree, 2, 0, 0, &stale));
    CHECK(bsp_tree_replace(tree, 2, 20));
    CHECK(!bsp_resize_update(tree, &stale, 10, 10));
    CHECK(bsp_resize_begin(tree, 20, 0, 0, &stale));
    CHECK(bsp_tree_sync(tree, NULL, 0, 0, bounds));
    CHECK(bsp_tree_sync(tree, ids, 4, 0, bounds));
    CHECK(!bsp_resize_update(tree, &stale, 10, 10));
    for (i = 0; i <= 129; ++i) {
        bounds = (BspRect){-20, 30, i, i / 2};
        bsp_tree_layout(tree, bounds, 32);
        test_tree(tree, 4);
    }
    bounds = (BspRect){INT_MAX - 3, INT_MAX - 1, INT_MAX, INT_MAX};
    bsp_tree_layout(tree, bounds, INT_MAX);
    CHECK(tree->bounds.width == 3 && tree->bounds.height == 1);
    test_tree(tree, 4);
    bounds = (BspRect){INT_MIN, INT_MIN, INT_MAX, INT_MAX};
    bsp_tree_layout(tree, bounds, INT_MAX);
    test_tree(tree, 4);
    bounds = (BspRect){0, 0, -1, -1};
    bsp_tree_layout(tree, bounds, 0);
    test_tree(tree, 4);
    CHECK(!bsp_resize_begin(tree, 1, 0, 0, &resize));
    bsp_forest_clear(&forest);
}
