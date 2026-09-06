#include "test.h"
#include "memory.h"

#include <string.h>

static int
parse(const char *text, struct bridge_memory *out)
{
    FILE *file = tmpfile();
    int result;
    CHECK(file != NULL);
    CHECK(fputs(text, file) != EOF);
    rewind(file);
    result = bridge_parse_meminfo(file, out);
    CHECK(fclose(file) == 0);
    return result;
}

static void
near(double actual, double expected)
{
    double difference = actual - expected;
    CHECK(difference < 0.0000001 && difference > -0.0000001);
}

void
test_memory(void)
{
    struct bridge_memory memory;
    const char *invalid[] = {
        "", "MemTotal: 0 kB\nMemAvailable: 0 kB\n",
        "MemAvailable: 100 kB\n", "MemTotal: 100 kB\n",
        "MemTotal: -100 kB\nMemAvailable: 50 kB\n",
        "MemTotal: +100 kB\nMemAvailable: 50 kB\n",
        "MemTotal: 100 MB\nMemAvailable: 50 kB\n",
        "MemTotal: 100kB\nMemAvailable: 50 kB\n",
        "MemTotal: 100 kB garbage\nMemAvailable: 50 kB\n",
        "MemTotal: 100.5 kB\nMemAvailable: 50 kB\n",
        "MemTotal: 18446744073709551616 kB\nMemAvailable: 50 kB\n",
        "MemTotal: 100 kB\nMemTotal: 100 kB\nMemAvailable: 50 kB\n",
        "MemTotal: 100 kB\nMemAvailable: 1 kB\nMemAvailable: 2 kB\n",
        "MemTotal: 100 kB\nMemAvailable: abc kB\n",
        "MemTotal: 100 kB\nMemAvailable: kB\n",
        "MemTotal: 100 kB\nMemAvailable: 50\n",
        "MemTotal: 100 kB\nMemFree: 10 kB\nBuffers: 10 kB\nCached: 10 kB\n",
        "MemTotal: 100 kB\nMemFree: 18446744073709551615 kB\n"
            "Buffers: 1 kB\nCached: 0 kB\nSReclaimable: 0 kB\nShmem: 0 kB\n"
    };
    char long_line[512];
    size_t i;

    CHECK(parse("MemTotal: 8388608 kB\nMemAvailable: 2097152 kB\n"
                "HugePages_Total: 0\nUnknown: nonsense\n", &memory));
    near(memory.percent, 75.0);
    near(memory.used_gib, 6.0);
    CHECK(parse("MemTotal: 8388608 kB\nMemFree: 1048576 kB\n"
                "Buffers: 524288 kB\nCached: 1048576 kB\n"
                "SReclaimable: 524288 kB\nShmem: 1048576 kB\n", &memory));
    near(memory.percent, 75.0);
    near(memory.used_gib, 6.0);
    CHECK(parse("MemTotal:\t1048576 kB\nMemAvailable: 0 kB", &memory));
    near(memory.percent, 100.0);
    near(memory.used_gib, 1.0);
    CHECK(parse("MemAvailable: 1048576 kB\nMemTotal: 1048576 kB\n", &memory));
    near(memory.percent, 0.0);
    near(memory.used_gib, 0.0);
    CHECK(parse("MemTotal: 100 kB\nMemAvailable: 101 kB\n", &memory));
    near(memory.percent, 0.0);
    CHECK(parse("MemTotal: 100 kB\nMemFree: 10 kB\nBuffers: 0 kB\n"
                "Cached: 0 kB\nSReclaimable: 0 kB\nShmem: 11 kB\n", &memory));
    near(memory.percent, 100.0);
    CHECK(parse("MemTotal: 100 kB\nMemFree: 101 kB\nBuffers: 0 kB\n"
                "Cached: 0 kB\nSReclaimable: 0 kB\nShmem: 0 kB\n", &memory));
    near(memory.percent, 0.0);
    CHECK(parse("MemTotal: 100 kB\nMemAvailable: 25 kB\nMemFree: 90 kB\n"
                "Buffers: 0 kB\nCached: 0 kB\nSReclaimable: 0 kB\nShmem: 0 kB\n",
                &memory));
    near(memory.percent, 75.0);
    CHECK(parse("MemTotal: 18446744073709551615 kB\nMemAvailable: 0 kB\n", &memory));
    near(memory.percent, 100.0);
    for (i = 0; i < sizeof invalid / sizeof invalid[0]; ++i) {
        memory.percent = -1;
        memory.used_gib = -1;
        CHECK(!parse(invalid[i], &memory));
        CHECK(memory.percent == -1 && memory.used_gib == -1);
    }
    memset(long_line, '9', sizeof long_line);
    memcpy(long_line, "MemTotal: ", 10);
    long_line[sizeof long_line - 1] = '\0';
    CHECK(!parse(long_line, &memory));
    /* Long unknown lines are skipped, never parsed as extra counters. */
    memset(long_line, 'x', 300);
    strcpy(long_line + 300, "\nMemTotal: 100 kB\nMemAvailable: 50 kB\n");
    CHECK(parse(long_line, &memory));
    near(memory.percent, 50.0);
}
