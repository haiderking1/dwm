#ifndef BAR_BRIDGE_TEST_H
#define BAR_BRIDGE_TEST_H
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
        exit(1); \
    } \
} while (0)

void test_commands(void);
void test_memory(void);
void test_state(void);
void test_transport(void);
#endif
