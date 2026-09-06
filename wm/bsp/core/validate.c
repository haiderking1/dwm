#include "internal.h"

struct validation {
    uint64_t *ids, *windows, maximum;
    size_t nodes, leaves;
};

static int validate_node(const BspNode *node, const BspNode *parent,
                         unsigned depth, struct validation *state)
{
    if (!node || depth > BSP_MAX_DEPTH || state->nodes >= BSP_MAX_NODES ||
        !node->id || node->parent != parent || !bsp_core_axis(node->axis) ||
        !bsp_core_ratio(node->ratio))
        return 0;
    state->ids[state->nodes++] = node->id;
    if (node->id > state->maximum)
        state->maximum = node->id;
    if (node->window) {
        if (node->child[0] || node->child[1] || state->leaves >= BSP_MAX_LEAVES)
            return 0;
        state->windows[state->leaves++] = node->window;
        return 1;
    }
    return validate_node(node->child[0], node, depth + 1, state) &&
           validate_node(node->child[1], node, depth + 1, state);
}

static int unique(uint64_t *ids, size_t count)
{
    size_t i;
    qsort(ids, count, sizeof *ids, bsp_core_compare_u64);
    for (i = 1; i < count; ++i)
        if (ids[i] == ids[i - 1])
            return 0;
    return 1;
}

int bsp_core_validate(const BspForest *forest, uint32_t *views, uint32_t *nodes)
{
    const BspWorkspace *view, *other;
    struct validation state;
    uint64_t *storage;
    size_t index;
    int ok = 0;
    *views = *nodes = 0;
    if (!forest)
        return 0;
    storage = malloc((BSP_MAX_NODES + BSP_MAX_LEAVES) * sizeof *storage);
    if (!storage)
        return 0;
    for (view = forest->views; view; view = view->next) {
        if (*views >= BSP_MAX_VIEWS || view->monitor < 0 || !view->tags ||
            view->tree.minimum <= 0 || !view->tree.next_id)
            goto finish;
        other = forest->views;
        for (index = 0; index < *views; ++index, other = other->next)
            if (other->monitor == view->monitor && other->tags == view->tags)
                goto finish;
        ++*views;
        memset(&state, 0, sizeof state);
        state.ids = storage;
        state.windows = storage + BSP_MAX_NODES;
        if (view->tree.root && !validate_node(view->tree.root, NULL, 0, &state))
            goto finish;
        if (view->tree.next_id <= state.maximum ||
            !unique(state.ids, state.nodes) || !unique(state.windows, state.leaves))
            goto finish;
        if (state.leaves ? !bsearch(&view->tree.focus, state.windows, state.leaves,
                                    sizeof *state.windows, bsp_core_compare_u64) :
                           view->tree.focus != 0)
            goto finish;
        if (state.nodes > BSP_MAX_FOREST_NODES - *nodes)
            goto finish;
        *nodes += (uint32_t)state.nodes;
    }
    ok = 1;
finish:
    free(storage);
    return ok;
}
