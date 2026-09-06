#ifndef BRIDGE_MEMORY_H
#define BRIDGE_MEMORY_H
#include <stdio.h>

struct bridge_memory {
    double percent;
    double used_gib;
};

/* Values in /proc/meminfo are KiB despite the kernel's "kB" spelling. */
int bridge_parse_meminfo(FILE *input, struct bridge_memory *out);
int bridge_read_memory(struct bridge_memory *out);
#endif
