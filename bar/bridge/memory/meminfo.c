#include "memory.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int
parse_value(const char *text, uint64_t *out)
{
    unsigned long long value;
    char *end;

    while (*text == ' ' || *text == '\t')
        ++text;
    if (*text < '0' || *text > '9')
        return 0;
    errno = 0;
    value = strtoull(text, &end, 10);
    if (errno == ERANGE || value > UINT64_MAX)
        return 0;
    if (*end != ' ' && *end != '\t')
        return 0;
    while (*end == ' ' || *end == '\t')
        ++end;
    if (strncmp(end, "kB", 2) != 0)
        return 0;
    end += 2;
    while (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')
        ++end;
    if (*end)
        return 0;
    *out = (uint64_t)value;
    return 1;
}

int
bridge_parse_meminfo(FILE *input, struct bridge_memory *out)
{
    static const char *const keys[] = {
        "MemTotal", "MemAvailable", "MemFree", "Buffers", "Cached",
        "SReclaimable", "Shmem"
    };
    uint64_t values[7] = {0}, available, used;
    unsigned int seen = 0;
    char line[256];
    size_t i;

    while (fgets(line, sizeof line, input)) {
        char *colon = strchr(line, ':');
        int truncated = !strchr(line, '\n') && !feof(input);
        if (colon) {
            *colon = '\0';
            for (i = 0; i < sizeof keys / sizeof keys[0]; ++i) {
                if (strcmp(line, keys[i]) != 0)
                    continue;
                if (truncated || (seen & (1U << i)) ||
                    !parse_value(colon + 1, &values[i]))
                    return 0;
                seen |= 1U << i;
                break;
            }
        }
        if (truncated) {
            int ch;
            do { ch = fgetc(input); } while (ch != '\n' && ch != EOF);
        }
    }
    if (ferror(input) || !(seen & 1U) || values[0] == 0)
        return 0;
    if (seen & 2U) {
        available = values[1];
    } else {
        if ((seen & 0x7cU) != 0x7cU)
            return 0;
        available = 0;
        for (i = 2; i <= 5; ++i) {
            if (values[i] > UINT64_MAX - available)
                return 0;
            available += values[i];
        }
        available = available > values[6] ? available - values[6] : 0;
    }
    /* Kernel counters can be sampled at slightly different instants. */
    if (available > values[0])
        available = values[0];
    used = values[0] - available;
    out->percent = 100.0 * (double)used / (double)values[0];
    out->used_gib = (double)used / 1048576.0;
    return 1;
}

int
bridge_read_memory(struct bridge_memory *out)
{
    FILE *input = fopen("/proc/meminfo", "r");
    int ok;
    if (!input)
        return 0;
    ok = bridge_parse_meminfo(input, out);
    if (fclose(input) != 0)
        ok = 0;
    return ok;
}
