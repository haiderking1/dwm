#ifndef DWM_BSP_CORE_INTERNAL_H
#define DWM_BSP_CORE_INTERNAL_H
#include "../bsp.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define BSP_MAX_LEAVES 4096u
#define BSP_MAX_NODES (2u * BSP_MAX_LEAVES - 1u)
#define BSP_MAX_DEPTH 128u
#define BSP_MAX_VIEWS 1024u
#define BSP_MAX_FOREST_NODES 131071u
#define BSP_RATIO_MIN 0.05
#define BSP_RATIO_MAX 0.95
#define BSP_DEFAULT_MINIMUM 32

int bsp_core_axis(BspAxis);
int bsp_core_ratio(double);
double bsp_core_clamp(double);
int bsp_core_rect_equal(BspRect, BspRect);
BspRect bsp_core_bounds(BspRect);
void bsp_core_changed(BspTree *);
void bsp_core_free(BspNode *);
void bsp_core_clear(BspTree *);
BspNode *bsp_core_first(BspNode *);
void bsp_core_remove(BspTree *, BspNode *);
size_t bsp_core_count(const BspNode *);
void bsp_core_minimum(const BspNode *, int, int64_t *, int64_t *);
double bsp_core_constrain(const BspTree *, const BspNode *, double);
int bsp_core_compare_u64(const void *, const void *);
int bsp_core_validate(const BspForest *, uint32_t *, uint32_t *);

#endif
