#include "test.h"
#include <math.h>

void test_operations(void)
{
    BspForest forest = {0};
    BspTree *tree = test_view(&forest, 0, 1);
    BspNode *one, *two, *parent;
    BspRect bounds = {0, 0, 1000, 600};
    uint64_t ids[] = {1, 2, 3}, generation;
    double ratio;
    CHECK(!bsp_tree_focus(tree, 1));
    CHECK(!bsp_tree_rotate(tree, 1));
    CHECK(bsp_tree_sync(tree, ids, 2, 0, bounds));
    CHECK(bsp_tree_focus(tree, 2));
    CHECK(bsp_tree_sync(tree, ids, 3, 0, bounds));
    two = bsp_find(tree, 2);
    parent = two->parent;
    CHECK(parent->axis == BSP_VERTICAL && parent->child[1]->window == 3);
    CHECK(tree->focus == 2);
    CHECK(!bsp_tree_focus(tree, 0) && !bsp_tree_focus(tree, 9));
    one = bsp_find(tree, 1);
    generation = tree->generation;
    CHECK(bsp_tree_swap(tree, 1, 2));
    CHECK(bsp_find(tree, 2) == one && bsp_find(tree, 1) == two);
    CHECK(tree->generation != generation && tree->focus == 2);
    CHECK(!bsp_tree_swap(tree, 1, 9));
    CHECK(bsp_tree_swap(tree, 1, 1));
    CHECK(!bsp_tree_replace(tree, 2, 0));
    CHECK(!bsp_tree_replace(tree, 2, 3));
    CHECK(bsp_tree_replace(tree, 2, UINT64_MAX));
    CHECK(tree->focus == UINT64_MAX && bsp_find(tree, UINT64_MAX) == one);
    CHECK(bsp_tree_replace(tree, UINT64_MAX, UINT64_MAX));
    CHECK(bsp_tree_adjust(tree, 1, BSP_VERTICAL, 0.1));
    ratio = parent->ratio;
    generation = tree->generation;
    CHECK(bsp_tree_rotate(tree, 1));
    CHECK(parent->axis == BSP_HORIZONTAL && parent->ratio == ratio);
    CHECK(tree->generation != generation);
    ratio = tree->root->ratio;
    CHECK(bsp_tree_adjust(tree, 1, BSP_HORIZONTAL, 0.1));
    CHECK(parent->ratio > 0.6 && tree->root->ratio == ratio);
    CHECK(!bsp_tree_adjust(tree, 1, BSP_VERTICAL, 0.1));
    CHECK(!bsp_tree_adjust(tree, 1, (BspAxis)7, 0.1));
    CHECK(!bsp_tree_adjust(tree, 1, BSP_HORIZONTAL, NAN));
    CHECK(!bsp_tree_adjust(tree, 1, BSP_HORIZONTAL, INFINITY));
    CHECK(bsp_tree_adjust(tree, 1, BSP_HORIZONTAL, 1e308));
    CHECK(parent->ratio <= 0.95);
    CHECK(bsp_tree_adjust(tree, 1, BSP_HORIZONTAL, -1e308));
    CHECK(parent->ratio >= 0.05);
    test_tree(tree, 3);
    bsp_tree_visit(NULL, NULL, NULL);
    bsp_tree_visit(tree, NULL, NULL);
    bsp_tree_layout(NULL, bounds, 32);
    CHECK(!bsp_find(NULL, 1) && !bsp_tree_sync(NULL, ids, 1, 0, bounds));
    bsp_forest_clear(&forest);
}
