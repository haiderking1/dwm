#include "wire.h"

static int read_node(FILE *stream, BspNode **out, BspNode *parent,
                      unsigned depth, uint32_t *remaining)
{
    unsigned char record[BSP_WIRE_NODE];
    BspNode *node;
    unsigned i;
    uint64_t window;
    double ratio;
    if (!*remaining || depth > BSP_MAX_DEPTH ||
        fread(record, 1, sizeof record, stream) != sizeof record)
        return 0;
    --*remaining;
    window = bsp_wire_u64(record + 8);
    ratio = bsp_wire_double(record + 16);
    if (!bsp_wire_u64(record) || !bsp_core_axis((BspAxis)record[24]) ||
        !bsp_core_ratio(ratio) || record[25] > 1 ||
        (record[25] == 1) != (window != 0))
        return 0;
    for (i = 26; i < sizeof record; ++i)
        if (record[i])
            return 0;
    node = calloc(1, sizeof *node);
    if (!node)
        return 0;
    *out = node;
    node->parent = parent;
    node->id = bsp_wire_u64(record);
    node->window = window;
    node->axis = (BspAxis)record[24];
    node->ratio = ratio;
    return window || (read_node(stream, &node->child[0], node, depth + 1, remaining) &&
                      read_node(stream, &node->child[1], node, depth + 1, remaining));
}

int bsp_forest_read(FILE *stream, BspForest *out)
{
    unsigned char header[BSP_WIRE_HEADER], record[BSP_WIRE_VIEW];
    BspForest temporary = {0};
    BspWorkspace *view, **tail = &temporary.views;
    uint32_t views, nodes, i, count, remaining, checked_views, checked_nodes;
    int ok = 0;
    if (!stream || !out || ferror(stream) ||
        fread(header, 1, sizeof header, stream) != sizeof header ||
        memcmp(header, bsp_wire_magic, sizeof bsp_wire_magic) ||
        bsp_wire_u32(header + 8) != BSP_WIRE_VERSION || bsp_wire_u32(header + 20))
        return 0;
    views = bsp_wire_u32(header + 12);
    nodes = bsp_wire_u32(header + 16);
    if (views > BSP_MAX_VIEWS || nodes > BSP_MAX_FOREST_NODES)
        return 0;
    for (i = 0; i < views; ++i) {
        if (fread(record, 1, sizeof record, stream) != sizeof record ||
            bsp_wire_u32(record) > INT_MAX || !bsp_wire_u32(record + 4) ||
            !bsp_wire_u32(record + 12) || bsp_wire_u32(record + 12) > INT_MAX)
            goto finish;
        count = bsp_wire_u32(record + 8);
        if (count > BSP_MAX_NODES || count > nodes || (count && !(count & 1)))
            goto finish;
        nodes -= count;
        view = calloc(1, sizeof *view);
        if (!view)
            goto finish;
        *tail = view;
        tail = &view->next;
        view->monitor = (int)bsp_wire_u32(record);
        view->tags = bsp_wire_u32(record + 4);
        view->tree.minimum = (int)bsp_wire_u32(record + 12);
        view->tree.focus = bsp_wire_u64(record + 16);
        view->tree.next_id = bsp_wire_u64(record + 24);
        remaining = count;
        if (count && (!read_node(stream, &view->tree.root, NULL, 0, &remaining) || remaining))
            goto finish;
    }
    if (nodes || ferror(stream) ||
        !bsp_core_validate(&temporary, &checked_views, &checked_nodes))
        goto finish;
    for (view = temporary.views; view; view = view->next)
        bsp_core_changed(&view->tree);
    bsp_forest_clear(out);
    *out = temporary;
    temporary.views = NULL;
    ok = 1;
finish:
    bsp_forest_clear(&temporary);
    return ok;
}
