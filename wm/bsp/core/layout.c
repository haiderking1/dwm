#include "internal.h"

int bsp_core_rect_equal(BspRect a, BspRect b)
{
    return a.x == b.x && a.y == b.y && a.width == b.width &&
           a.height == b.height;
}

BspRect bsp_core_bounds(BspRect rect)
{
    if (rect.width < 0)
        rect.width = 0;
    if (rect.height < 0)
        rect.height = 0;
    if ((int64_t)rect.x + rect.width > INT_MAX)
        rect.width = INT_MAX - rect.x;
    if ((int64_t)rect.y + rect.height > INT_MAX)
        rect.height = INT_MAX - rect.y;
    return rect;
}

void bsp_core_minimum(const BspNode *node, int minimum,
                      int64_t *width, int64_t *height)
{
    int64_t aw, ah, bw, bh;
    if (node->window) {
        *width = *height = minimum;
        return;
    }
    bsp_core_minimum(node->child[0], minimum, &aw, &ah);
    bsp_core_minimum(node->child[1], minimum, &bw, &bh);
    *width = node->axis == BSP_HORIZONTAL ? aw + bw : (aw > bw ? aw : bw);
    *height = node->axis == BSP_VERTICAL ? ah + bh : (ah > bh ? ah : bh);
}

static void needs(const BspNode *node, int minimum, int64_t *a, int64_t *b)
{
    int64_t aw, ah, bw, bh;
    bsp_core_minimum(node->child[0], minimum, &aw, &ah);
    bsp_core_minimum(node->child[1], minimum, &bw, &bh);
    *a = node->axis == BSP_HORIZONTAL ? aw : ah;
    *b = node->axis == BSP_HORIZONTAL ? bw : bh;
}

double bsp_core_constrain(const BspTree *tree, const BspNode *node, double ratio)
{
    int span = node->axis == BSP_HORIZONTAL ? node->rect.width : node->rect.height;
    int64_t a, b;
    double lower, upper;
    ratio = bsp_core_clamp(ratio);
    if (span <= 0)
        return ratio;
    needs(node, tree->minimum, &a, &b);
    lower = (double)a / span;
    upper = 1.0 - (double)b / span;
    if (lower < BSP_RATIO_MIN)
        lower = BSP_RATIO_MIN;
    if (upper > BSP_RATIO_MAX)
        upper = BSP_RATIO_MAX;
    if (lower <= upper)
        return ratio < lower ? lower : ratio > upper ? upper : ratio;
    return ratio;
}

static void layout(BspNode *node, BspRect rect, int minimum)
{
    int span, cut;
    int64_t a, b;
    BspRect first = rect, second = rect;
    node->rect = rect;
    if (node->window)
        return;
    span = node->axis == BSP_HORIZONTAL ? rect.width : rect.height;
    needs(node, minimum, &a, &b);
    if (a + b > span) {
        /* Divide first: span * a could exceed int64_t for huge minimums. */
        cut = (int)((double)span * ((double)a / (double)(a + b)) + 0.5);
    } else {
        cut = (int)((double)span * bsp_core_clamp(node->ratio) + 0.5);
        if (cut < a)
            cut = (int)a;
        if ((int64_t)span - cut < b)
            cut = span - (int)b;
    }
    if (node->axis == BSP_HORIZONTAL) {
        first.width = cut;
        second.x += cut;
        second.width -= cut;
    } else {
        first.height = cut;
        second.y += cut;
        second.height -= cut;
    }
    layout(node->child[0], first, minimum);
    layout(node->child[1], second, minimum);
}

void bsp_tree_layout(BspTree *tree, BspRect bounds, int minimum)
{
    if (!tree)
        return;
    bounds = bsp_core_bounds(bounds);
    if (minimum <= 0)
        minimum = BSP_DEFAULT_MINIMUM;
    if (!bsp_core_rect_equal(tree->bounds, bounds) || tree->minimum != minimum)
        bsp_core_changed(tree);
    tree->bounds = bounds;
    tree->minimum = minimum;
    if (tree->root)
        layout(tree->root, bounds, minimum);
}
