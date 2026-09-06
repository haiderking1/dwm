#include "internal.h"

BspWorkspace *
bsp_workspace(BspForest *forest, int monitor, uint32_t tags, int create)
{
    BspWorkspace *view;
    size_t count = 0;
    if (!forest || monitor < 0 || !tags)
        return NULL;
    for (view = forest->views; view; view = view->next) {
        if (view->monitor == monitor && view->tags == tags)
            return view;
        ++count;
    }
    if (!create || count >= BSP_MAX_VIEWS)
        return NULL;
    view = calloc(1, sizeof *view);
    if (!view)
        return NULL;
    view->monitor = monitor;
    view->tags = tags;
    view->tree.minimum = BSP_DEFAULT_MINIMUM;
    view->tree.next_id = 1;
    bsp_core_changed(&view->tree);
    view->next = forest->views;
    forest->views = view;
    return view;
}

void bsp_forest_clear(BspForest *forest)
{
    BspWorkspace *view, *next;
    if (!forest)
        return;
    for (view = forest->views; view; view = next) {
        next = view->next;
        bsp_core_clear(&view->tree);
        free(view);
    }
    forest->views = NULL;
}

void bsp_forget_window(BspForest *forest, uint64_t window)
{
    BspWorkspace *view;
    BspNode *leaf;
    if (!forest || !window)
        return;
    for (view = forest->views; view; view = view->next) {
        leaf = bsp_find(&view->tree, window);
        if (leaf) {
            bsp_core_remove(&view->tree, leaf);
            bsp_tree_layout(&view->tree, view->tree.bounds,
                            view->tree.minimum);
        }
    }
}

void bsp_forget_monitor(BspForest *forest, int monitor)
{
    BspWorkspace **link, *view;
    if (!forest || monitor < 0)
        return;
    for (link = &forest->views; (view = *link); ) {
        if (view->monitor != monitor) {
            link = &view->next;
            continue;
        }
        *link = view->next;
        bsp_core_clear(&view->tree);
        free(view);
    }
}
