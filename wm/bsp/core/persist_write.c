#include "wire.h"

static int write_node(FILE *stream, const BspNode *node)
{
    unsigned char record[BSP_WIRE_NODE] = {0};
    if (!node)
        return 1;
    bsp_wire_put64(record, node->id);
    bsp_wire_put64(record + 8, node->window);
    bsp_wire_put_double(record + 16, node->ratio);
    record[24] = (unsigned char)node->axis;
    record[25] = (unsigned char)(node->window != 0);
    if (fwrite(record, 1, sizeof record, stream) != sizeof record)
        return 0;
    return write_node(stream, node->child[0]) && write_node(stream, node->child[1]);
}

int bsp_forest_write(FILE *stream, const BspForest *forest)
{
    unsigned char header[BSP_WIRE_HEADER] = {0}, record[BSP_WIRE_VIEW];
    const BspWorkspace *view;
    uint32_t views, nodes;
    if (!stream || ferror(stream) || !bsp_core_validate(forest, &views, &nodes))
        return 0;
    memcpy(header, bsp_wire_magic, sizeof bsp_wire_magic);
    bsp_wire_put32(header + 8, BSP_WIRE_VERSION);
    bsp_wire_put32(header + 12, views);
    bsp_wire_put32(header + 16, nodes);
    if (fwrite(header, 1, sizeof header, stream) != sizeof header)
        return 0;
    for (view = forest->views; view; view = view->next) {
        bsp_wire_put32(record, (uint32_t)view->monitor);
        bsp_wire_put32(record + 4, view->tags);
        bsp_wire_put32(record + 8, (uint32_t)bsp_core_count(view->tree.root));
        bsp_wire_put32(record + 12, (uint32_t)view->tree.minimum);
        bsp_wire_put64(record + 16, view->tree.focus);
        bsp_wire_put64(record + 24, view->tree.next_id);
        if (fwrite(record, 1, sizeof record, stream) != sizeof record ||
            !write_node(stream, view->tree.root))
            return 0;
    }
    return !ferror(stream);
}
