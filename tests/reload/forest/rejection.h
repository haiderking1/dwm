static FILE *
checkpoint_prefix(FILE *source, long length)
{
	FILE *copy = tmpfile();
	long i;
	int byte;
	assert(copy);
	rewind(source);
	for (i = 0; i < length; i++) {
		byte = fgetc(source);
		assert(byte != EOF && fputc(byte, copy) != EOF);
	}
	return copy;
}

static long
checkpoint_length(FILE *file)
{
	long length;
	assert(fseek(file, 0, SEEK_END) == 0);
	length = ftell(file);
	assert(length >= 0);
	return length;
}

static void
assert_retry_succeeds(FILE *source, WeightFixture *fixture)
{
	BspForest expected = {0};
	unsigned int applies = forest_applies;
	FILE *copy = checkpoint_prefix(source, checkpoint_length(source));
	assert(restore_checkpoint(copy));
	assert(forest_applies == applies + 1);
	populate_forest(&expected);
	assert_trees(&checkpoint_forest, &expected);
	bsp_forest_clear(&expected);
	assert(selmon == &fixture->monitors[1]);
	assert(mons->tagset[0] == 4 && mons->mfact == .65f);
	assert(mons->clients == &fixture->clients[3]);
	assert(mons->stack == &fixture->clients[3]);
	assert(mons->sel == &fixture->clients[1]);
	assert(fixture->clients[0].cfact == weights[0]);
	assert(fixture->clients[2].mon == selmon && fixture->clients[2].tags == 16);
}

static void
test_forest_truncation(void)
{
	WeightFixture fixture;
	FILE *source, *bad;
	long offset, length, end;
	forest_fixture_init(&fixture);
	source = weight_checkpoint();
	offset = forest_offset(source);
	length = checkpoint_length(source);
	assert(length > offset);
	/* Every missing/partial forest field, including the final byte. */
	for (end = offset; end < length; end++) {
		bad = checkpoint_prefix(source, end);
		simulate_scan(&fixture);
		assert_rejected_unchanged(bad, &fixture);
		assert_retry_succeeds(source, &fixture);
	}
	assert(fclose(source) == 0);
}

static void
test_forest_trailing_data(void)
{
	WeightFixture fixture;
	FILE *source, *bad;
	forest_fixture_init(&fixture);
	source = weight_checkpoint();
	bad = checkpoint_prefix(source, checkpoint_length(source));
	assert(fputc('!', bad) != EOF);
	simulate_scan(&fixture);
	assert_rejected_unchanged(bad, &fixture);
	assert_retry_succeeds(source, &fixture);
	assert(fclose(source) == 0);
}

static void
test_wrapper_mask_validation(void)
{
	WeightFixture fixture;
	BspForest invalid = {0};
	FILE *source, *bad;
	forest_fixture_init(&fixture);
	source = weight_checkpoint();
	bad = checkpoint_prefix(source, forest_offset(source));
	populate_forest(&invalid);
	/* Valid to the generic engine, invalid for this WM's configured tags. */
	invalid.views->tags = TAGMASK + 1U;
	assert(bsp_forest_write(bad, &invalid));
	bsp_forest_clear(&invalid);
	simulate_scan(&fixture);
	assert_rejected_unchanged(bad, &fixture);
	assert_retry_succeeds(source, &fixture);
	assert(fclose(source) == 0);
}

static void
test_invalid_prefix_with_valid_forest(void)
{
	WeightFixture fixture;
	FILE *source, *bad;
	const float invalid = NAN;
	long offsets[3];
	unsigned int i;
	forest_fixture_init(&fixture);
	source = weight_checkpoint();
	offsets[0] = forest_offset(source) - sizeof(float);
	offsets[1] = sizeof(ReloadHeader) + offsetof(ReloadMonitor, mfact);
	offsets[2] = sizeof(ReloadHeader) + sizeof(ReloadMonitor) * 2 +
	             offsetof(ReloadClient, w);
	for (i = 0; i < LENGTH(offsets); i++) {
		const int width = 0;
		bad = checkpoint_prefix(source, checkpoint_length(source));
		assert(fseek(bad, offsets[i], SEEK_SET) == 0);
		if (i == 2) assert(fwrite(&width, sizeof width, 1, bad) == 1);
		else assert(fwrite(&invalid, sizeof invalid, 1, bad) == 1);
		simulate_scan(&fixture);
		assert_rejected_unchanged(bad, &fixture);
		assert_retry_succeeds(source, &fixture);
	}
	assert(fclose(source) == 0);
}
