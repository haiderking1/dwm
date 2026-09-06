/* Core wire V1 uses a 24-byte header, 32-byte views and preorder 32-byte
 * nodes. Keep format-dependent mutations here; roundtrips and truncation
 * tests use only the public writer. Recheck this against persist_*.c when
 * the engine's wire version changes. */
static uint32_t
fixture_u32(const unsigned char *bytes)
{
	return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 |
	       (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
}

static void
reject_forest_patch(FILE *source, WeightFixture *fixture, long offset,
                    const unsigned char *bytes, size_t size)
{
	FILE *bad = checkpoint_prefix(source, checkpoint_length(source));
	assert(fseek(bad, offset, SEEK_SET) == 0);
	assert(fwrite(bytes, 1, size, bad) == size);
	simulate_scan(fixture);
	assert_rejected_unchanged(bad, fixture);
	assert_retry_succeeds(source, fixture);
}

static void
test_forest_corruption(void)
{
	WeightFixture fixture;
	FILE *source;
	unsigned char *wire;
	const unsigned char zero[8] = {0}, one[8] = {1}, two[8] = {2};
	const unsigned char maximum[8] = {255,255,255,255,255,255,255,255};
	const unsigned char nan_ratio[8] = {0,0,0,0,0,0,248,127};
	const unsigned char infinity[8] = {0,0,0,0,0,0,240,127};
	long base, size, position, views[4], root, leaves[3];
	unsigned int i, j, count, leaf_count = 0;
	forest_fixture_init(&fixture);
	source = weight_checkpoint();
	base = forest_offset(source);
	size = checkpoint_length(source) - base;
	wire = malloc((size_t)size);
	assert(wire && fseek(source, base, SEEK_SET) == 0);
	assert(fread(wire, 1, (size_t)size, source) == (size_t)size);
	assert(size >= 24 && fixture_u32(wire + 8) == 1);
	assert(fixture_u32(wire + 12) == LENGTH(views));
	position = 24;
	for (i = 0; i < LENGTH(views); i++) {
		assert(position + 32 <= size);
		views[i] = position;
		count = fixture_u32(wire + position + 8);
		position += 32;
		assert(position + count * 32 <= size);
		position += count * 32;
	}
	assert(position == size);
	/* Corrupt the last view so earlier trees have already been allocated. */
	root = views[3] + 32;
	count = fixture_u32(wire + views[3] + 8);
	assert(count == 5 && wire[root + 25] == 0);
	for (j = 0; j < count; j++)
		if (wire[root + j * 32 + 25] == 1) {
			assert(leaf_count < LENGTH(leaves));
			leaves[leaf_count++] = root + j * 32;
		}
	assert(leaf_count == LENGTH(leaves));
#define REJECT(at, bytes, length) \
	reject_forest_patch(source, &fixture, base + (at), bytes, length)
	wire[0] ^= 255;
	REJECT(0, wire, 1);                         /* magic */
	REJECT(8, two, 4);                         /* unknown version */
	REJECT(12, maximum, 4);                    /* unbounded view count */
	REJECT(16, zero, 4);                       /* total nodes mismatch */
	REJECT(20, one, 4);                        /* reserved header */
	REJECT(views[3], maximum, 4);              /* negative monitor */
	REJECT(views[3] + 4, zero, 4);             /* empty tag mask */
	REJECT(views[3], wire + views[0], 8);      /* duplicate workspace key */
	REJECT(views[3] + 8, two, 4);              /* even node count */
	REJECT(views[3] + 12, zero, 4);            /* invalid minimum */
	REJECT(views[3] + 16, maximum, 8);         /* nonexistent focus */
	REJECT(views[3] + 24, one, 8);             /* exhausted node identity */
	REJECT(root, zero, 8);                    /* zero node identity */
	REJECT(root + 16, zero, 8);               /* zero split ratio */
	REJECT(root + 16, nan_ratio, 8);
	REJECT(root + 16, infinity, 8);
	REJECT(root + 24, two, 1);                /* invalid axis */
	REJECT(root + 25, two, 1);                /* invalid node kind */
	REJECT(root + 25, one, 1);                /* branch marked as leaf */
	REJECT(root + 26, one, 1);                /* reserved node byte */
	REJECT(leaves[0], wire + root, 8);        /* duplicate node identity */
	REJECT(leaves[0] + 8, zero, 8);           /* zero window identity */
	REJECT(leaves[1] + 8, wire + leaves[0] + 8, 8); /* duplicate leaf */
#undef REJECT
	free(wire);
	assert(fclose(source) == 0);
}
