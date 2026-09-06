#include "test.h"
#include <math.h>

static void count_leaf(uint64_t id, BspRect rect, void *context)
{
    size_t *count = context;
    (void)id; (void)rect;
    ++*count;
}

static void invalid_write(const BspForest *forest)
{
    FILE *stream = tmpfile();
    CHECK(stream);
    CHECK(!bsp_forest_write(stream, forest));
    CHECK(ftell(stream) == 0 && fclose(stream) == 0);
}

void test_corruption(void)
{
    BspForest source = {0}, output = {0};
    BspTree *tree = test_view(&source, 0, 1);
    BspRect bounds = {0, 0, 1000, 800};
    BspWorkspace *original;
    BspNode *root, *leaf, *saved;
    uint64_t ids[] = {1, 2, 3, 4}, saved_id, saved_window, saved_focus, generation;
    double ratio;
    BspAxis axis;
    unsigned char *bytes, *mutated;
    size_t length, count;
    unsigned step, edit;
    uint32_t rng = UINT32_C(0x9e3779b9);
    int result;
    CHECK(bsp_tree_sync(tree, ids, 4, 0, bounds));
    bytes = test_bytes(&source, &length);
    mutated = malloc(length);
    CHECK(mutated);
    for (step = 0; step < 4096; ++step) {
        if (!output.views)
            test_view(&output, 99, 2);
        original = output.views;
        generation = original->tree.generation;
        memcpy(mutated, bytes, length);
        for (edit = 0; edit <= step % 4; ++edit) {
            rng = rng * UINT32_C(1664525) + UINT32_C(1013904223);
            mutated[rng % length] ^= (unsigned char)(1u << ((rng >> 16) % 8));
        }
        result = test_read_bytes(mutated, length, &output);
        if (!result) {
            CHECK(output.views == original && original->tree.generation == generation);
        } else {
            BspWorkspace *view;
            for (view = output.views; view; view = view->next) {
                count = 0;
                bsp_tree_layout(&view->tree, bounds, view->tree.minimum);
                bsp_tree_visit(&view->tree, count_leaf, &count);
                test_tree(&view->tree, count);
            }
        }
    }
    free(mutated);
    free(bytes);
    root = tree->root;
    leaf = bsp_find(tree, 1);
    ratio = root->ratio;
    root->ratio = NAN;
    invalid_write(&source);
    root->ratio = ratio;
    axis = root->axis;
    root->axis = (BspAxis)2;
    invalid_write(&source);
    root->axis = axis;
    saved_id = leaf->id;
    leaf->id = root->id;
    invalid_write(&source);
    leaf->id = saved_id;
    saved_window = leaf->window;
    leaf->window = 2;
    invalid_write(&source);
    leaf->window = saved_window;
    saved = root->child[1];
    root->child[1] = root;
    invalid_write(&source);
    root->child[1] = saved;
    root->child[1] = root->child[0];
    invalid_write(&source);
    root->child[1] = saved;
    saved_focus = tree->focus;
    tree->focus = 99;
    invalid_write(&source);
    tree->focus = saved_focus;
    source.views->next = source.views;
    invalid_write(&source);
    source.views->next = NULL;
    test_tree(tree, 4);
    bsp_forest_clear(&source);
    bsp_forest_clear(&output);
}
