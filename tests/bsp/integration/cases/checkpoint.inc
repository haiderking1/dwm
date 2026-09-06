static void
test_checkpoint(void)
{
	BspWorkspace *view = mouse_setup(), *saved;
	BspForest restored = {0}, invalid = {0}, unchanged = {0};
	FILE *stream;
	uint64_t ghost = 909;

	view->tree.root->ratio = 0.71;
	stream = tmpfile(); assert(stream); assert(bsp_checkpoint_write(stream));
	rewind(stream); assert(bsp_checkpoint_read(stream, &restored)); fclose(stream);
	saved = bsp_workspace(&restored, 0, 1, 0); assert(saved);
	assert(saved->tree.root->ratio == 0.71 && saved->tree.focus == view->tree.focus);
	bsp_forest_clear(&restored);
	/* Dormant views are serialized, with stale tags/floats/monitors pruned. */
	selmon->tagset[0] = 3; arrange(selmon);
	selmon->tagset[0] = 1;
	wintoclient(2)->tags = 2;
	saved = bsp_workspace(&bsp_forest, 99, 1, 1); assert(saved);
	assert(bsp_tree_sync(&saved->tree, &ghost, 1, 0, bsp_monitor_bounds(selmon)));
	stream = tmpfile(); assert(stream);
	assert(bsp_checkpoint_write(stream));
	assert(!bsp_workspace(&bsp_forest, 99, 1, 0));
	assert(!bsp_find(&view->tree, 2));
	rewind(stream); assert(bsp_checkpoint_read(stream, &restored)); fclose(stream);
	assert(restored.views && restored.views != bsp_forest.views);
	saved = bsp_workspace(&restored, 0, 3, 0); assert(saved);
	assert(bsp_find(&saved->tree, 1) && bsp_find(&saved->tree, 2));
	/* Apply transfers ownership, prunes clients absent from real restore, and
	 * leaves the caller's temporary empty for unconditional cleanup. */
	remove_client(2);
	bsp_checkpoint_apply(&restored); assert(!restored.views);
	saved = bsp_workspace(&bsp_forest, 0, 3, 0); assert(saved);
	assert(bsp_find(&saved->tree, 1) && !bsp_find(&saved->tree, 2));
	/* Valid engine data with an invalid dwm tag mask must not mutate out/live. */
	saved = bsp_workspace(&invalid, 0, 1u << 20, 1); assert(saved);
	assert(bsp_tree_sync(&saved->tree, &ghost, 1, 0, bsp_monitor_bounds(selmon)));
	saved = bsp_workspace(&unchanged, 8, 1, 1); assert(saved);
	stream = tmpfile(); assert(stream);
	assert(bsp_forest_write(stream, &invalid)); rewind(stream);
	view = bsp_forest.views;
	assert(!bsp_checkpoint_read(stream, &unchanged)); fclose(stream);
	assert(unchanged.views == saved && bsp_forest.views == view);
	bsp_forest_clear(&unchanged); bsp_forest_clear(&invalid);
	/* Missing monitors are removed only on apply, not during detached read. */
	stream = tmpfile(); assert(stream); assert(bsp_checkpoint_write(stream));
	rewind(stream); assert(bsp_checkpoint_read(stream, &restored)); fclose(stream);
	selmon->num = 5;
	bsp_checkpoint_apply(&restored); assert(!restored.views && !bsp_forest.views);
}
