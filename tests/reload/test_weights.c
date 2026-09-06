#include <limits.h>
#include <math.h>
#include <stddef.h>
#include "checkpoint/fixture.h"

static void
test_v3_prefix_and_matching(void)
{
	WeightFixture fixture;
	FILE *file, *expected;
	float actual[LENGTH(weights)];
	BspForest parsed = {0};
	int byte;
	weight_fixture_init(&fixture);
	file = weight_checkpoint();
	expected = legacy_checkpoint(3);
	rewind(expected);
	/* Compare every old byte, not just fields decoded by the new structs. */
	while ((byte = fgetc(expected)) != EOF) assert(fgetc(file) == byte);
	assert(!ferror(expected));
	assert(ftell(file) == legacy_size());
	assert(fclose(expected) == 0);
	assert(fread(actual, sizeof actual, 1, file) == 1);
	assert(memcmp(actual, weights, sizeof weights) == 0);
	assert(bsp_forest_read(file, &parsed) && !parsed.views);
	bsp_forest_clear(&parsed);
	assert(fgetc(file) == EOF && !ferror(file));
	change_live_state(&fixture);
	assert(restore_checkpoint(file));
	assert(fixture.clients[0].cfact == weights[0]);
	assert(fixture.clients[2].cfact == weights[2]);
	assert(fixture.clients[1].cfact == 9.0f); /* No longer managed. */
	assert(fixture.clients[3].cfact == 1.0f); /* Not in the checkpoint. */
	assert(selmon == &fixture.monitors[1]);
	assert(mons->clients == &fixture.clients[0]);
	assert(mons->clients->next == &fixture.clients[3]);
}

static void
test_legacy_migration(unsigned int version)
{
	WeightFixture fixture;
	FILE *file;
	unsigned int i;
	weight_fixture_init(&fixture);
	/* Frozen records model the running V1/V2 binary, without a forest. */
	file = legacy_checkpoint(version);
	if (version == 2) assert(fwrite(weights, sizeof weights, 1, file) == 1);
	assert(ftell(file) == legacy_size() + (version == 2 ? (long)sizeof weights : 0));
	assert(bsp_workspace(&checkpoint_forest, 0, 1, 1));
	selmon = mons;
	mons->mfact = .85f;
	for (i = 0; i < LENGTH(legacy_clients); i++) fixture.clients[i].cfact = 9.0f;
	fixture.clients[3].cfact = 6.0f;
	attach(&fixture.clients[3]);
	attachstack(&fixture.clients[3]);
	assert(restore_checkpoint(file));
	assert(!checkpoint_forest.views && forest_applies == 1);
	for (i = 0; i < LENGTH(legacy_clients); i++)
		assert(fixture.clients[i].cfact == (version == 1 ? 1.0f : weights[i]));
	assert(fixture.clients[3].cfact == 6.0f);
	assert(selmon == &fixture.monitors[1] && mons->mfact == .65f);
	assert(mons->clients == &fixture.clients[0]);
	assert(mons->clients->next == &fixture.clients[1]);
	assert(fixture.clients[1].next == &fixture.clients[3]);
	assert(mons->stack == &fixture.clients[1]);
	assert(mons->stack->snext == &fixture.clients[0]);
	assert(mons->sel == &fixture.clients[1]);
	assert(fixture.clients[0].isfloating && fixture.clients[0].oldstate);
	assert(fixture.clients[2].mon == selmon && fixture.clients[2].tags == 16);
}

static void
test_invalid_factors(void)
{
	const float invalid[] = { NAN, INFINITY, -INFINITY, -1.0f, 0.0f,
	                         .000999f, 1000001.0f };
	WeightFixture fixture;
	FILE *file;
	unsigned int i, slot;
	for (i = 0; i < LENGTH(invalid); i++)
		for (slot = 0; slot < LENGTH(weights); slot++) {
			weight_fixture_init(&fixture);
			file = weight_checkpoint();
			assert(fseek(file, legacy_size() + slot * sizeof(float), SEEK_SET) == 0);
			assert(fwrite(&invalid[i], sizeof invalid[i], 1, file) == 1);
			change_live_state(&fixture);
			assert_rejected_unchanged(file, &fixture);
		}
}

static void
test_truncated_factors(void)
{
	WeightFixture fixture;
	FILE *file;
	size_t bytes;
	/* Include an absent trailer, whole missing factors, and partial floats. */
	for (bytes = 0; bytes < sizeof weights; bytes++) {
		weight_fixture_init(&fixture);
		file = weight_checkpoint();
		assert(fflush(file) == 0);
		assert(ftruncate(fileno(file), legacy_size() + bytes) == 0);
		change_live_state(&fixture);
		assert_rejected_unchanged(file, &fixture);
	}
}

static void
test_unknown_versions(void)
{
	const unsigned int versions[] = { 0, 4, UINT_MAX };
	WeightFixture fixture;
	FILE *file;
	unsigned int i;
	for (i = 0; i < LENGTH(versions); i++) {
		weight_fixture_init(&fixture);
		file = weight_checkpoint();
		assert(fseek(file, offsetof(LegacyHeader, version), SEEK_SET) == 0);
		assert(fwrite(&versions[i], sizeof versions[i], 1, file) == 1);
		change_live_state(&fixture);
		assert_rejected_unchanged(file, &fixture);
	}
}

static void
test_factor_boundaries_and_empty_checkpoint(void)
{
	WeightFixture fixture;
	FILE *file;
	unsigned int i;
	BspForest parsed = {0};
	weight_fixture_init(&fixture);
	fixture.clients[0].cfact = .001f;
	fixture.clients[1].cfact = 1000000.0f;
	file = weight_checkpoint();
	fixture.clients[0].cfact = fixture.clients[1].cfact = 1.0f;
	assert(restore_checkpoint(file));
	assert(fixture.clients[0].cfact == .001f);
	assert(fixture.clients[1].cfact == 1000000.0f);
	for (i = 0; i < LENGTH(legacy_clients); i++) {
		detach(&fixture.clients[i]);
		detachstack(&fixture.clients[i]);
	}
	file = weight_checkpoint();
	assert(fseek(file, sizeof legacy_header + sizeof legacy_monitors, SEEK_SET) == 0);
	assert(bsp_forest_read(file, &parsed) && !parsed.views);
	bsp_forest_clear(&parsed);
	assert(fgetc(file) == EOF && !ferror(file));
	assert(restore_checkpoint(file));
	assert(!mons->clients && !selmon->clients);
}

int
main(void)
{
	test_v3_prefix_and_matching();
	test_legacy_migration(1);
	test_legacy_migration(2);
	test_invalid_factors();
	test_truncated_factors();
	test_unknown_versions();
	test_factor_boundaries_and_empty_checkpoint();
	bsp_forest_clear(&checkpoint_forest);
	puts("reload weight tests passed");
	return 0;
}
