#include "internal.h"

static int contains(const uint64_t *ids, size_t count, uint64_t id)
{
    return count && bsearch(&id, ids, count, sizeof *ids,
                           bsp_core_compare_u64) != NULL;
}

static BspNode *prune(BspNode *node, const uint64_t *ids, size_t count,
                      int *changed)
{
    BspNode *survivor;
    if (!node)
        return NULL;
    if (node->window) {
        if (contains(ids, count, node->window))
            return node;
        free(node);
        *changed = 1;
        return NULL;
    }
    node->child[0] = prune(node->child[0], ids, count, changed);
    node->child[1] = prune(node->child[1], ids, count, changed);
    if (node->child[0] && node->child[1])
        return node;
    survivor = node->child[node->child[0] == NULL];
    if (survivor)
        survivor->parent = node->parent;
    free(node);
    return survivor;
}

static int insert(BspTree *tree, uint64_t window, uint64_t anchor)
{
    BspNode *leaf, *branch = NULL, *target, *parent, *walk;
    unsigned depth = 0;
    uint64_t needed;
    target = bsp_find(tree, anchor);
    if (!target)
        target = bsp_find(tree, tree->focus);
    if (!target)
        target = bsp_core_first(tree->root);
    for (walk = target; walk && walk->parent; walk = walk->parent)
        ++depth;
    if (target && depth >= BSP_MAX_DEPTH)
        return 0;
    needed = target ? 2 : 1;
    if (!tree->next_id)
        tree->next_id = 1;
    if (tree->next_id > UINT64_MAX - needed)
        return 0;
    leaf = calloc(1, sizeof *leaf);
    if (!leaf)
        return 0;
    if (target) {
        branch = calloc(1, sizeof *branch);
        if (!branch) {
            free(leaf);
            return 0;
        }
    }
    leaf->id = tree->next_id++;
    leaf->window = window;
    leaf->ratio = 0.5;
    if (!target) {
        tree->root = leaf;
    } else {
        parent = target->parent;
        branch->id = tree->next_id++;
        branch->ratio = 0.5;
        branch->axis = target->rect.width >= target->rect.height ?
                       BSP_HORIZONTAL : BSP_VERTICAL;
        branch->parent = parent;
        branch->child[0] = target;
        branch->child[1] = leaf;
        target->parent = leaf->parent = branch;
        if (parent)
            parent->child[parent->child[1] == target] = branch;
        else
            tree->root = branch;
    }
    if (!tree->focus)
        tree->focus = window;
    bsp_core_changed(tree);
    bsp_tree_layout(tree, tree->bounds, tree->minimum);
    return 1;
}

int bsp_tree_sync(BspTree *tree, const uint64_t *windows, size_t count,
                  uint64_t anchor, BspRect bounds)
{
    uint64_t *sorted = NULL;
    BspNode *first;
    size_t i;
    int changed = 0;
    if (!tree || count > BSP_MAX_LEAVES || (count && !windows))
        return 0;
    if (count) {
        sorted = malloc(count * sizeof *sorted);
        if (!sorted)
            return 0;
        memcpy(sorted, windows, count * sizeof *sorted);
        qsort(sorted, count, sizeof *sorted, bsp_core_compare_u64);
        for (i = 0; i < count; ++i)
            if (!sorted[i] || (i && sorted[i] == sorted[i - 1])) {
                free(sorted);
                return 0;
            }
    }
    tree->root = prune(tree->root, sorted, count, &changed);
    free(sorted);
    if (!bsp_find(tree, tree->focus)) {
        first = bsp_core_first(tree->root);
        tree->focus = first ? first->window : 0;
    }
    if (changed || !tree->generation)
        bsp_core_changed(tree);
    bsp_tree_layout(tree, bounds, tree->minimum);
    for (i = 0; i < count; ++i)
        if (!bsp_find(tree, windows[i]) && !insert(tree, windows[i], anchor))
            return 0;
    return 1;
}

static void visit(const BspNode *node, BspLeafFn callback, void *context)
{
    if (!node)
        return;
    if (node->window)
        callback(node->window, node->rect, context);
    else {
        visit(node->child[0], callback, context);
        visit(node->child[1], callback, context);
    }
}

void bsp_tree_visit(const BspTree *tree, BspLeafFn callback, void *context)
{
    if (tree && callback)
        visit(tree->root, callback, context);
}
