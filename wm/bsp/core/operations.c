#include "internal.h"
#include <float.h>

int bsp_tree_focus(BspTree *tree, uint64_t window)
{
    if (!bsp_find(tree, window))
        return 0;
    if (tree->focus != window) {
        tree->focus = window;
        bsp_core_changed(tree);
    }
    return 1;
}

int bsp_tree_swap(BspTree *tree, uint64_t a, uint64_t b)
{
    BspNode *first = bsp_find(tree, a), *second = bsp_find(tree, b);
    if (!first || !second)
        return 0;
    if (first != second) {
        first->window = b;
        second->window = a;
        bsp_core_changed(tree);
    }
    return 1;
}

int bsp_tree_replace(BspTree *tree, uint64_t old_window, uint64_t new_window)
{
    BspNode *leaf = bsp_find(tree, old_window);
    if (!leaf || !new_window)
        return 0;
    if (old_window == new_window)
        return 1;
    if (bsp_find(tree, new_window))
        return 0;
    leaf->window = new_window;
    if (tree->focus == old_window)
        tree->focus = new_window;
    bsp_core_changed(tree);
    return 1;
}

int bsp_tree_rotate(BspTree *tree, uint64_t window)
{
    BspNode *leaf = bsp_find(tree, window), *parent;
    if (!leaf || !(parent = leaf->parent))
        return 0;
    parent->axis = parent->axis == BSP_HORIZONTAL ?
                   BSP_VERTICAL : BSP_HORIZONTAL;
    bsp_core_changed(tree);
    bsp_tree_layout(tree, tree->bounds, tree->minimum);
    return 1;
}

int bsp_tree_adjust(BspTree *tree, uint64_t window, BspAxis axis, double delta)
{
    BspNode *node = bsp_find(tree, window);
    double ratio;
    if (!node || !bsp_core_axis(axis) || !(delta >= -DBL_MAX && delta <= DBL_MAX))
        return 0;
    for (node = node->parent; node && node->axis != axis; node = node->parent)
        ;
    if (!node)
        return 0;
    /* Clamp delta before addition so finite extremes cannot overflow. */
    ratio = delta < -1 ? BSP_RATIO_MIN : delta > 1 ? BSP_RATIO_MAX :
            node->ratio + delta;
    node->ratio = bsp_core_constrain(tree, node, ratio);
    bsp_tree_layout(tree, tree->bounds, tree->minimum);
    return 1;
}
