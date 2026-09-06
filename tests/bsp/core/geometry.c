#include "test.h"

static void at_least_32(uint64_t window, BspRect rect, void *context)
{
    (void)window;
    (void)context;
    CHECK(rect.width >= 32 && rect.height >= 32);
}

void test_geometry(void)
{
    BspForest forest = {0};
    BspTree *tree = test_view(&forest, 0, 1);
    BspRect bounds = {23, -31, 1600, 1200}, sibling;
    uint64_t ids[] = {1, 2, 3, 4, 5, 6};
    BspNode *root, *branch;
    BspResize resize, untouched;
    uint64_t generation;
    int width;
    /* A stale wide rectangle must not select the insertion axis. */
    CHECK(bsp_tree_sync(tree, ids, 1, 0, (BspRect){0, 0, 900, 100}));
    CHECK(bsp_tree_sync(tree, ids, 2, 1, (BspRect){0, 0, 100, 900}));
    CHECK(tree->root->axis == BSP_VERTICAL && tree->root->child[1]->window == 2);
    CHECK(bsp_tree_sync(tree, NULL, 0, 0, bounds));
    CHECK(bsp_tree_sync(tree, ids, 2, 1, bounds));
    CHECK(bsp_tree_sync(tree, ids, 3, 2, bounds));
    CHECK(bsp_tree_sync(tree, ids, 4, 2, bounds));
    CHECK(bsp_tree_sync(tree, ids, 5, 2, bounds));
    CHECK(bsp_tree_focus(tree, 3));
    CHECK(bsp_tree_sync(tree, ids, 6, 999, bounds));
    CHECK(bsp_find(tree, 3)->parent->child[1]->window == 6);
    root = tree->root;
    CHECK(bsp_tree_adjust(tree, 1, BSP_HORIZONTAL, 100));
    /* The right subtree needs at least 64 pixels, not merely one leaf's 32. */
    CHECK(root->child[1]->rect.width >= 64);
    bsp_tree_visit(tree, at_least_32, NULL);
    CHECK(bsp_tree_adjust(tree, 2, BSP_HORIZONTAL, -100));
    CHECK(bsp_tree_adjust(tree, 2, BSP_VERTICAL, 100));
    bsp_tree_visit(tree, at_least_32, NULL);
    test_tree(tree, 6);
    branch = bsp_find(tree, 2)->parent;
    sibling = branch->child[1]->rect;
    generation = tree->generation;
    /* Unchanged arrangements preserve stored ratios and active drag tokens. */
    CHECK(bsp_resize_begin(tree, 2, 1500, 50, &resize));
    bsp_tree_layout(tree, bounds, 32);
    CHECK(tree->generation == generation);
    CHECK(bsp_tree_sync(tree, ids, 6, 0, bounds));
    CHECK(tree->generation == generation);
    CHECK(bsp_resize_update(tree, &resize, 1500, 50));
    CHECK(branch->child[1]->rect.width == sibling.width);
    /* Failed begin does not expose a partially populated baseline. */
    memset(&untouched, 0x5a, sizeof untouched);
    resize = untouched;
    CHECK(!bsp_resize_begin(tree, 999, 0, 0, &resize));
    CHECK(!memcmp(&resize, &untouched, sizeof resize));
    CHECK(!bsp_resize_begin(tree, 2, 0, 0, NULL));
    CHECK(!bsp_resize_update(tree, NULL, 0, 0));
    CHECK(bsp_resize_begin(tree, 2, 0, 0, &resize));
    resize.horizontal = UINT64_MAX;
    CHECK(!bsp_resize_update(tree, &resize, 10, 10));
    bsp_forest_clear(&forest);
    tree = test_view(&forest, 0, 1);
    bounds = (BspRect){-7, -11, 1025, 201};
    CHECK(bsp_tree_sync(tree, ids, 2, 1, bounds));
    CHECK(bsp_tree_adjust(tree, 1, BSP_HORIZONTAL, -0.17));
    width = bsp_find(tree, 1)->rect.width;
    CHECK(width == 338 && bsp_find(tree, 2)->rect.width == 687);
    CHECK(bsp_resize_begin(tree, 1, width - 7, 80, &resize));
    CHECK(bsp_resize_update(tree, &resize, width - 7 + 73, 80));
    CHECK(bsp_find(tree, 1)->rect.width == width + 73);
    CHECK(bsp_find(tree, 2)->rect.width == 1025 - width - 73);
    test_tree(tree, 2);
    /* A newly allocated view must reject a token from a forgotten view. */
    CHECK(bsp_resize_begin(tree, 1, 0, 0, &resize));
    bsp_forget_monitor(&forest, 0);
    tree = test_view(&forest, 0, 1);
    CHECK(bsp_tree_sync(tree, ids, 2, 1, bounds));
    CHECK(!bsp_resize_update(tree, &resize, 1, 1));
    bsp_forest_clear(&forest);
}
