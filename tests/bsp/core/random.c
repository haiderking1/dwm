#include "test.h"

static uint32_t state = UINT32_C(0xc6a4a793);
static uint32_t random32(void)
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

static void count_leaf(uint64_t id, BspRect rect, void *context)
{
    size_t *count = context;
    (void)id;
    (void)rect;
    ++*count;
}

static void membership(const BspTree *tree, const unsigned char *present)
{
    uint64_t id;
    size_t count = 0;
    for (id = 1; id <= 48; ++id) {
        CHECK((bsp_find(tree, id) != NULL) == (present[id] != 0));
        count += present[id] != 0;
    }
    test_tree(tree, count);
}

void test_random(void)
{
    BspForest forest = {0};
    BspTree *tree;
    BspRect bounds[4];
    unsigned char present[4][49] = {{0}};
    uint64_t ids[48], a, b;
    BspResize resize;
    unsigned step, view, op, i, j;
    size_t count, length;
    unsigned char *bytes;
    for (view = 0; view < 4; ++view) {
        tree = test_view(&forest, (int)(view / 2), 1u << (view % 2));
        bounds[view] = (BspRect){(int)view * 31, -20, 1301, 877};
        CHECK(bsp_tree_sync(tree, NULL, 0, 0, bounds[view]));
    }
    for (step = 0; step < 20000; ++step) {
        view = random32() % 4;
        tree = &bsp_workspace(&forest, (int)(view / 2), 1u << (view % 2), 0)->tree;
        a = random32() % 48 + 1;
        b = random32() % 48 + 1;
        op = random32() % 10;
        if (op <= 2) {
            present[view][a] ^= 1;
            count = 0;
            for (i = 1; i <= 48; ++i)
                if (present[view][i])
                    ids[count++] = i;
            for (i = (unsigned)count; i > 1; --i) {
                uint64_t tmp;
                j = random32() % i;
                tmp = ids[j]; ids[j] = ids[i - 1]; ids[i - 1] = tmp;
            }
            CHECK(bsp_tree_sync(tree, ids, count, b, bounds[view]));
        } else if (op == 3) {
            CHECK(bsp_tree_focus(tree, a) == (present[view][a] != 0));
        } else if (op == 4) {
            CHECK(bsp_tree_swap(tree, a, b) == (present[view][a] && present[view][b]));
        } else if (op == 5) {
            bsp_tree_rotate(tree, a);
            bsp_tree_adjust(tree, b, (BspAxis)(random32() % 2),
                            ((int)(random32() % 41) - 20) / 100.0);
        } else if (op == 6) {
            int x = (int)(random32() % 2000) - 300;
            int y = (int)(random32() % 2000) - 300;
            if (bsp_resize_begin(tree, a, x, y, &resize)) {
                CHECK(bsp_resize_update(tree, &resize, x + 53, y - 27));
                CHECK(bsp_resize_update(tree, &resize, x - 99, y + 83));
            }
        } else if (op == 7) {
            int valid = present[view][a] && (a == b || !present[view][b]);
            CHECK(bsp_tree_replace(tree, a, b) == valid);
            if (valid) {
                present[view][a] = 0;
                present[view][b] = 1;
            }
        } else if (op == 8) {
            bounds[view].width = (int)(random32() % 1500);
            bounds[view].height = (int)(random32() % 1000);
            bsp_tree_layout(tree, bounds[view], 32);
        } else {
            bsp_forget_window(&forest, a);
            for (i = 0; i < 4; ++i)
                present[i][a] = 0;
        }
        for (i = 0; i < 4; ++i)
            membership(&bsp_workspace(&forest, (int)(i / 2), 1u << (i % 2), 0)->tree,
                       present[i]);
        if (!(step % 251)) {
            bytes = test_bytes(&forest, &length);
            CHECK(test_read_bytes(bytes, length, &forest));
            free(bytes);
            for (i = 0; i < 4; ++i) {
                tree = &bsp_workspace(&forest, (int)(i / 2), 1u << (i % 2), 0)->tree;
                bsp_tree_layout(tree, bounds[i], 32);
                membership(tree, present[i]);
            }
        }
    }
    count = 0;
    bsp_tree_visit(&forest.views->tree, count_leaf, &count);
    test_tree(&forest.views->tree, count);
    bsp_forest_clear(&forest);
}
