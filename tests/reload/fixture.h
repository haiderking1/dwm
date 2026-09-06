#ifndef RELOAD_TEST_FIXTURE_H
#define RELOAD_TEST_FIXTURE_H
#include <stddef.h>
void fixture_write(const char *path, const char *text);
void fixture_path(char *out, size_t size, const char *dir, const char *name);
void fixture_wait(const char *path);
int fixture_probe(int argc, char **argv);
#endif
