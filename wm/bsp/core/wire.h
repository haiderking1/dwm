#ifndef DWM_BSP_CORE_WIRE_H
#define DWM_BSP_CORE_WIRE_H
#include "internal.h"
#define BSP_WIRE_VERSION 1u
#define BSP_WIRE_HEADER 24u
#define BSP_WIRE_VIEW 32u
#define BSP_WIRE_NODE 32u
extern const unsigned char bsp_wire_magic[8];
uint32_t bsp_wire_u32(const unsigned char *);
uint64_t bsp_wire_u64(const unsigned char *);
void bsp_wire_put32(unsigned char *, uint32_t);
void bsp_wire_put64(unsigned char *, uint64_t);
double bsp_wire_double(const unsigned char *);
void bsp_wire_put_double(unsigned char *, double);
#endif
