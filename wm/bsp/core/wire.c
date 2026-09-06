#include "wire.h"
#include <float.h>
#if DBL_MANT_DIG != 53 || DBL_MAX_EXP != 1024 || DBL_MIN_EXP != -1021
#error BSP persistence requires IEEE binary64 double
#endif
typedef char bsp_double_must_be_eight_bytes[sizeof(double) == 8 ? 1 : -1];

const unsigned char bsp_wire_magic[8] = {'B','S','P','F','R','S','T',0};

uint32_t bsp_wire_u32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

uint64_t bsp_wire_u64(const unsigned char *p)
{
    return (uint64_t)bsp_wire_u32(p) | (uint64_t)bsp_wire_u32(p + 4) << 32;
}

void bsp_wire_put32(unsigned char *p, uint32_t value)
{
    unsigned i;
    for (i = 0; i < 4; ++i)
        p[i] = (unsigned char)(value >> (8 * i));
}

void bsp_wire_put64(unsigned char *p, uint64_t value)
{
    bsp_wire_put32(p, (uint32_t)value);
    bsp_wire_put32(p + 4, (uint32_t)(value >> 32));
}

double bsp_wire_double(const unsigned char *p)
{
    uint64_t bits = bsp_wire_u64(p);
    double value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

void bsp_wire_put_double(unsigned char *p, double value)
{
    uint64_t bits;
    memcpy(&bits, &value, sizeof bits);
    bsp_wire_put64(p, bits);
}
