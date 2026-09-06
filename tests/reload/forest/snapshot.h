#ifndef RELOAD_FOREST_SNAPSHOT_H
#define RELOAD_FOREST_SNAPSHOT_H
static FILE *
forest_snapshot(const BspForest *forest)
{
	FILE *file = tmpfile();
	assert(file);
	assert(bsp_forest_write(file, forest));
	assert(fflush(file) == 0);
	rewind(file);
	return file;
}

static void
assert_forest_snapshot(FILE *expected, const BspForest *forest)
{
	FILE *actual = forest_snapshot(forest);
	int byte;
	rewind(expected);
	while ((byte = fgetc(expected)) != EOF) assert(fgetc(actual) == byte);
	assert(!ferror(expected));
	assert(fgetc(actual) == EOF && !ferror(actual));
	assert(fclose(actual) == 0);
}
#endif
