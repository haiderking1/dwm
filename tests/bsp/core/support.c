#include "test.h"

void test_fail(const char *expression, const char *file, int line)
{
    fprintf(stderr, "%s:%d: %s\n", file, line, expression);
    abort();
}

BspTree *test_view(BspForest *forest, int monitor, uint32_t tags)
{
    BspWorkspace *view = bsp_workspace(forest, monitor, tags, 1);
    CHECK(view);
    return &view->tree;
}

struct check_state {
    uint64_t ids[8191], windows[4096];
    size_t nodes, leaves;
    int64_t area;
};

static int same_rect(BspRect a, BspRect b)
{
    return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}

static void check_node(const BspTree *tree, const BspNode *node,
                        const BspNode *parent, unsigned depth,
                        struct check_state *state)
{
    size_t i;
    BspRect a, b, rect = node->rect;
    CHECK(depth <= 128 && state->nodes < 8191);
    CHECK(node->id && node->id < tree->next_id && node->parent == parent);
    CHECK(node->axis == BSP_HORIZONTAL || node->axis == BSP_VERTICAL);
    CHECK(node->ratio >= 0.05 && node->ratio <= 0.95);
    CHECK(rect.width >= 0 && rect.height >= 0);
    CHECK((int64_t)rect.x + rect.width <= INT_MAX);
    CHECK((int64_t)rect.y + rect.height <= INT_MAX);
    for (i = 0; i < state->nodes; ++i)
        CHECK(state->ids[i] != node->id);
    state->ids[state->nodes++] = node->id;
    if (node->window) {
        CHECK(!node->child[0] && !node->child[1] && state->leaves < 4096);
        for (i = 0; i < state->leaves; ++i)
            CHECK(state->windows[i] != node->window);
        state->windows[state->leaves++] = node->window;
        state->area += (int64_t)rect.width * rect.height;
        CHECK(bsp_find(tree, node->window) == node);
        return;
    }
    CHECK(node->child[0] && node->child[1]);
    a = node->child[0]->rect;
    b = node->child[1]->rect;
    CHECK(a.x == rect.x && a.y == rect.y);
    if (node->axis == BSP_HORIZONTAL) {
        CHECK(a.height == rect.height && b.height == rect.height && b.y == rect.y);
        CHECK((int64_t)a.width + b.width == rect.width && b.x == a.x + a.width);
    } else {
        CHECK(a.width == rect.width && b.width == rect.width && b.x == rect.x);
        CHECK((int64_t)a.height + b.height == rect.height && b.y == a.y + a.height);
    }
    check_node(tree, node->child[0], node, depth + 1, state);
    check_node(tree, node->child[1], node, depth + 1, state);
}

static void visited(uint64_t window, BspRect rect, void *context)
{
    size_t *count = context;
    CHECK(window && rect.width >= 0 && rect.height >= 0);
    ++*count;
}

void test_tree(const BspTree *tree, size_t leaves)
{
    struct check_state *state = calloc(1, sizeof *state);
    size_t visits = 0;
    CHECK(state);
    CHECK(tree->generation && tree->minimum > 0);
    if (tree->root) {
        CHECK(same_rect(tree->root->rect, tree->bounds));
        check_node(tree, tree->root, NULL, 0, state);
        CHECK(state->area == (int64_t)tree->bounds.width * tree->bounds.height);
        CHECK(bsp_find(tree, tree->focus));
    } else {
        CHECK(!tree->focus);
    }
    CHECK(state->leaves == leaves);
    CHECK(state->nodes == (leaves ? 2 * leaves - 1 : 0));
    bsp_tree_visit(tree, visited, &visits);
    CHECK(visits == leaves);
    free(state);
}

static void equal_node(const BspNode *a, const BspNode *b)
{
    CHECK((a != NULL) == (b != NULL));
    if (!a)
        return;
    CHECK(a->id == b->id && a->window == b->window && a->axis == b->axis);
    CHECK(a->ratio == b->ratio);
    equal_node(a->child[0], b->child[0]);
    equal_node(a->child[1], b->child[1]);
}

void test_equal(const BspTree *a, const BspTree *b)
{
    CHECK(a->focus == b->focus && a->next_id == b->next_id && a->minimum == b->minimum);
    equal_node(a->root, b->root);
}

unsigned char *test_bytes(const BspForest *forest, size_t *length)
{
    FILE *stream = tmpfile();
    unsigned char *bytes;
    long end;
    CHECK(stream && bsp_forest_write(stream, forest));
    CHECK(fflush(stream) == 0);
    end = ftell(stream);
    CHECK(end >= 0);
    *length = (size_t)end;
    bytes = malloc(*length);
    CHECK(bytes);
    rewind(stream);
    CHECK(fread(bytes, 1, *length, stream) == *length);
    CHECK(fclose(stream) == 0);
    return bytes;
}

int test_read_bytes(const unsigned char *bytes, size_t length, BspForest *out)
{
    FILE *stream = tmpfile();
    int result;
    CHECK(stream);
    CHECK(fwrite(bytes, 1, length, stream) == length);
    rewind(stream);
    result = bsp_forest_read(stream, out);
    CHECK(fclose(stream) == 0);
    return result;
}

void test_put32(unsigned char *p, uint32_t value)
{
    unsigned i;
    for (i = 0; i < 4; ++i)
        p[i] = (unsigned char)(value >> (8 * i));
}

void test_put64(unsigned char *p, uint64_t value)
{
    test_put32(p, (uint32_t)value);
    test_put32(p + 4, (uint32_t)(value >> 32));
}
