/* Frozen V1 wire records. Do not replace these with production typedefs. */
typedef struct {
	unsigned int magic, version, monitors, clients;
	int selected_monitor;
} LegacyHeader;
typedef struct {
	int num, nmaster, showbar, topbar;
	unsigned int tagset[2], seltags, sellt, layout[2];
	float mfact;
	Window selected;
} LegacyMonitor;
typedef struct {
	Window window;
	int monitor, x, y, w, h, oldx, oldy, oldw, oldh;
	int floating, fullscreen, oldstate;
	unsigned int tags, stackorder;
} LegacyClient;

static const LegacyHeader legacy_header = { 0x44574d52U, 1, 2, 3, 1 };
static const LegacyMonitor legacy_monitors[] = {
	{ 0, 2, 1, 1, {4, 2}, 0, 1, {2, 1}, .65f, 10 },
	{ 1, 1, 0, 0, {8, 16}, 1, 0, {0, 2}, .55f, 20 }
};
static const LegacyClient legacy_clients[] = {
	{ 30, 0, 120, 80, 600, 400, 100, 60, 500, 300, 1, 0, 1, 4, 1 },
	{ 10, 0, 20, 40, 700, 500, 10, 30, 600, 400, 0, 0, 0, 4, 0 },
	{ 20, 1, 2020, 60, 800, 600, 2000, 40, 700, 500, 1, 0, 0, 16, 0 }
};

static long
legacy_size(void)
{
	return sizeof legacy_header + sizeof legacy_monitors + sizeof legacy_clients;
}

static FILE *
legacy_checkpoint(unsigned int version)
{
	LegacyHeader header = legacy_header;
	FILE *file = tmpfile();
	assert(file);
	header.version = version;
	assert(fwrite(&header, sizeof header, 1, file) == 1);
	assert(fwrite(legacy_monitors, sizeof legacy_monitors, 1, file) == 1);
	assert(fwrite(legacy_clients, sizeof legacy_clients, 1, file) == 1);
	assert(ftell(file) == legacy_size());
	return file;
}
