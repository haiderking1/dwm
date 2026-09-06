#ifndef DWM_BSP_H
#define DWM_BSP_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* Pure tree engine: X11 window IDs are opaque, nonzero leaf identities. */
typedef struct { int x, y, width, height; } BspRect;
typedef enum { BSP_HORIZONTAL = 0, BSP_VERTICAL = 1 } BspAxis;
/* Horizontal partitions left/right; vertical partitions top/bottom. */
typedef struct BspNode BspNode;
struct BspNode {
    uint64_t id, window;
    BspAxis axis;
    double ratio;
    BspRect rect;
    BspNode *parent, *child[2];
};
typedef struct {
    BspNode *root;
    uint64_t focus, generation, next_id;
    BspRect bounds;
    int minimum;
} BspTree;
typedef struct BspWorkspace BspWorkspace;
struct BspWorkspace {
    int monitor;
    uint32_t tags;
    BspTree tree;
    BspWorkspace *next;
};
typedef struct { BspWorkspace *views; } BspForest;
typedef struct {
    uint64_t generation, window, horizontal, vertical;
    BspRect bounds;
    int start_x, start_y, horizontal_span, vertical_span;
    double horizontal_ratio, vertical_ratio;
} BspResize;
typedef void (*BspLeafFn)(uint64_t window, BspRect rect, void *context);

BspWorkspace *bsp_workspace(BspForest *, int monitor, uint32_t tags, int create);
void bsp_forest_clear(BspForest *);
void bsp_forget_window(BspForest *, uint64_t window);
void bsp_forget_monitor(BspForest *, int monitor);
BspNode *bsp_find(const BspTree *, uint64_t window);
/* Reconcile membership, preserving surviving branches/ratios. New leaves split
 * the existing anchor, otherwise the last focused leaf, otherwise a leaf.
 * Bounds must be laid out before choosing the wider-axis insertion direction.
 * Duplicate/zero identities are invalid. Failed allocation leaves a valid tree. */
int bsp_tree_sync(BspTree *, const uint64_t *windows, size_t count,
                  uint64_t anchor, BspRect bounds);
void bsp_tree_layout(BspTree *, BspRect bounds, int minimum);
void bsp_tree_visit(const BspTree *, BspLeafFn, void *context);
int bsp_tree_focus(BspTree *, uint64_t window);
int bsp_tree_swap(BspTree *, uint64_t a, uint64_t b);
int bsp_tree_replace(BspTree *, uint64_t old_window, uint64_t new_window);
int bsp_tree_rotate(BspTree *, uint64_t window);
int bsp_tree_adjust(BspTree *, uint64_t window, BspAxis axis, double delta);
/* Resize only internal boundaries touching the selected leaf. Baselines contain
 * identities, never node pointers. Topology/generation or bounds changes cancel. */
int bsp_resize_begin(const BspTree *, uint64_t window, int x, int y, BspResize *);
int bsp_resize_update(BspTree *, const BspResize *, int x, int y);
/* Versioned, bounded serialization. Read validates into temporary ownership and
 * leaves output unchanged on failure. No pointers/geometry caches are persisted. */
int bsp_forest_write(FILE *, const BspForest *);
int bsp_forest_read(FILE *, BspForest *);
#endif
