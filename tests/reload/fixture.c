#define _POSIX_C_SOURCE 200809L
#include "fixture.h"
#include <assert.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void
fixture_write(const char *path, const char *text)
{
	FILE *file = fopen(path, "w");
	assert(file);
	assert(fputs(text, file) >= 0);
	assert(fclose(file) == 0);
}

void
fixture_path(char *out, size_t size, const char *dir, const char *name)
{
	int n = snprintf(out, size, "%s/%s", dir, name);
	assert(n > 0 && (size_t)n < size);
}

void
fixture_wait(const char *path)
{
	for (int i = 0; i < 500; ++i) {
		if (access(path, F_OK) == 0)
			return;
		assert(poll(NULL, 0, 10) >= 0);
	}
	fprintf(stderr, "timed out waiting for %s\n", path);
	abort();
}

int
fixture_probe(int argc, char **argv)
{
	char ready[1024], release[1024], complete[1024], session[64];
	const char *dir = getenv("RELOAD_TEST_DIR");
	assert(argc == 3 && dir);
	assert(strcmp(argv[2], getenv("RELOAD_TEST_PREFIX")) == 0);
	fixture_path(ready, sizeof ready, dir, "ready");
	fixture_path(release, sizeof release, dir, "release");
	fixture_path(complete, sizeof complete, dir, "complete");
	assert(snprintf(session, sizeof session, "%ld\n", (long)getsid(0)) > 0);
	fixture_write(ready, session);
	fixture_wait(release);
	fixture_write(complete, "complete\n");
	puts("probe stdout");
	fputs("probe stderr\n", stderr);
	return 0;
}
