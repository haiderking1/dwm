#include "internal.h"

static int touches(const BspNode *node, const BspNode *leaf)
{
    BspRect a = node->rect, b = leaf->rect;
    int edge;
    if (!b.width || !b.height)
        return 0;
    if (node->axis == BSP_HORIZONTAL) {
        edge = node->child[1]->rect.x;
        return (edge == b.x || edge == b.x + b.width) &&
               b.y < a.y + a.height && a.y < b.y + b.height;
    }
    edge = node->child[1]->rect.y;
    return (edge == b.y || edge == b.y + b.height) &&
           b.x < a.x + a.width && a.x < b.x + b.width;
}

int bsp_resize_begin(const BspTree *tree, uint64_t window, int x, int y,
                      BspResize *out)
{
    BspNode *leaf = bsp_find(tree, window), *node;
    BspResize resize = {0};
    int64_t best[2] = {INT64_MAX, INT64_MAX}, distance;
    int edge, span;
    if (!out || !leaf || !tree->generation || tree->generation == UINT64_MAX)
        return 0;
    for (node = leaf->parent; node; node = node->parent) {
        if (!touches(node, leaf))
            continue;
        span = node->axis == BSP_HORIZONTAL ? node->rect.width : node->rect.height;
        if (!span)
            continue;
        edge = node->axis == BSP_HORIZONTAL ? node->child[1]->rect.x :
                                             node->child[1]->rect.y;
        distance = (int64_t)(node->axis == BSP_HORIZONTAL ? x : y) - edge;
        if (distance < 0)
            distance = -distance;
        if (distance >= best[node->axis])
            continue;
        best[node->axis] = distance;
        /* Use the actual pixel cut, not a ratio hidden by minimum clamping.
         * This makes the first release delta move the visible boundary. */
        if (node->axis == BSP_HORIZONTAL) {
            resize.horizontal = node->id;
            resize.horizontal_span = span;
            resize.horizontal_ratio = (double)(edge - node->rect.x) / span;
        } else {
            resize.vertical = node->id;
            resize.vertical_span = span;
            resize.vertical_ratio = (double)(edge - node->rect.y) / span;
        }
    }
    if (!resize.horizontal && !resize.vertical)
        return 0;
    resize.generation = tree->generation;
    resize.window = window;
    resize.bounds = tree->bounds;
    resize.start_x = x;
    resize.start_y = y;
    *out = resize;
    return 1;
}

static BspNode *boundary(BspNode *leaf, uint64_t id, BspAxis axis)
{
    BspNode *node;
    for (node = leaf->parent; node; node = node->parent)
        if (node->id == id)
            return node->axis == axis && touches(node, leaf) ? node : NULL;
    return NULL;
}

static int baseline(double ratio, int span)
{
    /* Effective cuts can be outside the stored ratio bounds. */
    return span > 0 && ratio >= 0 && ratio <= 1;
}

int bsp_resize_update(BspTree *tree, const BspResize *resize, int x, int y)
{
    BspNode *leaf, *horizontal = NULL, *vertical = NULL;
    double ratio;
    if (!tree || !resize || !resize->generation ||
        tree->generation == UINT64_MAX || tree->generation != resize->generation ||
        !bsp_core_rect_equal(tree->bounds, resize->bounds) ||
        !(leaf = bsp_find(tree, resize->window)) ||
        (!resize->horizontal && !resize->vertical))
        return 0;
    if (resize->horizontal) {
        horizontal = boundary(leaf, resize->horizontal, BSP_HORIZONTAL);
        if (!horizontal || !baseline(resize->horizontal_ratio, resize->horizontal_span))
            return 0;
    }
    if (resize->vertical) {
        vertical = boundary(leaf, resize->vertical, BSP_VERTICAL);
        if (!vertical || !baseline(resize->vertical_ratio, resize->vertical_span))
            return 0;
    }
    if (horizontal) {
        ratio = resize->horizontal_ratio +
                ((double)x - resize->start_x) / resize->horizontal_span;
        horizontal->ratio = bsp_core_constrain(tree, horizontal, ratio);
    }
    if (vertical) {
        ratio = resize->vertical_ratio +
                ((double)y - resize->start_y) / resize->vertical_span;
        vertical->ratio = bsp_core_constrain(tree, vertical, ratio);
    }
    bsp_tree_layout(tree, tree->bounds, tree->minimum);
    return 1;
}
