#include "internal.h"

/* Tokens are process-wide, including replacement forests and empty trees.
 * At exhaustion resize is disabled rather than accepting reused tokens. */
static uint64_t generation;

void
bsp_core_changed(BspTree *tree)
{
    if (generation != UINT64_MAX)
        ++generation;
    tree->generation = generation;
}

int bsp_core_axis(BspAxis axis)
{
    return axis == BSP_HORIZONTAL || axis == BSP_VERTICAL;
}

int bsp_core_ratio(double ratio)
{
    return ratio >= BSP_RATIO_MIN && ratio <= BSP_RATIO_MAX;
}

double bsp_core_clamp(double ratio)
{
    return ratio < BSP_RATIO_MIN ? BSP_RATIO_MIN :
           ratio > BSP_RATIO_MAX ? BSP_RATIO_MAX : ratio;
}

int bsp_core_compare_u64(const void *a, const void *b)
{
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}

void bsp_core_free(BspNode *node)
{
    if (!node)
        return;
    bsp_core_free(node->child[0]);
    bsp_core_free(node->child[1]);
    free(node);
}

void bsp_core_clear(BspTree *tree)
{
    bsp_core_free(tree->root);
    tree->root = NULL;
    tree->focus = 0;
    bsp_core_changed(tree);
}

BspNode *bsp_core_first(BspNode *node)
{
    while (node && node->child[0])
        node = node->child[0];
    return node;
}

BspNode *bsp_find(const BspTree *tree, uint64_t window)
{
    BspNode *stack[BSP_MAX_DEPTH + 1], *node;
    size_t used = 0;
    if (!tree || !window || !tree->root)
        return NULL;
    stack[used++] = tree->root;
    while (used) {
        node = stack[--used];
        if (node->window == window)
            return node;
        if (node->child[0]) {
            stack[used++] = node->child[1];
            stack[used++] = node->child[0];
        }
    }
    return NULL;
}

size_t bsp_core_count(const BspNode *node)
{
    return node ? 1 + bsp_core_count(node->child[0]) +
                      bsp_core_count(node->child[1]) : 0;
}

void bsp_core_remove(BspTree *tree, BspNode *leaf)
{
    BspNode *parent = leaf->parent, *sibling, *grand;
    uint64_t window = leaf->window;
    if (!parent) {
        tree->root = NULL;
    } else {
        sibling = parent->child[parent->child[0] == leaf];
        grand = parent->parent;
        sibling->parent = grand;
        if (grand)
            grand->child[grand->child[1] == parent] = sibling;
        else
            tree->root = sibling;
        free(parent);
    }
    free(leaf);
    if (tree->focus == window) {
        leaf = bsp_core_first(tree->root);
        tree->focus = leaf ? leaf->window : 0;
    }
    bsp_core_changed(tree);
}
