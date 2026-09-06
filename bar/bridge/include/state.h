#ifndef BRIDGE_STATE_H
#define BRIDGE_STATE_H
#include <stddef.h>
#include <stdio.h>

#define BRIDGE_STATE_LIMIT (1024UL * 1024UL)
/* Framing guard, not a JSON parser: the root property is trusted dwm JSON. */
int bridge_state_valid(const unsigned char *data, size_t length);
int bridge_write_state(FILE *output, const unsigned char *data, size_t length);
#endif
