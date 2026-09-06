#include "test.h"
#include "state.h"

#include <string.h>

static void
check_frame(const unsigned char *input, size_t length, const char *expected)
{
    FILE *file = tmpfile();
    char output[256];
    size_t count;
    CHECK(file != NULL);
    CHECK(bridge_write_state(file, input, length));
    rewind(file);
    count = fread(output, 1, sizeof output - 1, file);
    output[count] = '\0';
    CHECK(strcmp(output, expected) == 0);
    CHECK(fclose(file) == 0);
}

void
test_state(void)
{
    const unsigned char valid[] = "{\"title\":\"a\\nb\\r\\t\\u0000\"}";
    const unsigned char nul[] = {'{', '\0', '}'};
    unsigned char *large = malloc(BRIDGE_STATE_LIMIT + 1);
    CHECK(large != NULL);
    CHECK(bridge_state_valid(valid, sizeof valid - 1));
    CHECK(!bridge_state_valid(NULL, 0));
    CHECK(!bridge_state_valid((const unsigned char *)"", 0));
    CHECK(!bridge_state_valid((const unsigned char *)"{", 1));
    CHECK(!bridge_state_valid((const unsigned char *)"[]", 2));
    CHECK(!bridge_state_valid((const unsigned char *)"{}\n", 3));
    CHECK(!bridge_state_valid((const unsigned char *)"{\n}", 3));
    CHECK(!bridge_state_valid((const unsigned char *)"{\r}", 3));
    CHECK(!bridge_state_valid(nul, sizeof nul));
    CHECK(!bridge_state_valid((const unsigned char *)"{}x", 3));
    memset(large, ' ', BRIDGE_STATE_LIMIT + 1);
    large[0] = '{';
    large[BRIDGE_STATE_LIMIT - 1] = '}';
    CHECK(bridge_state_valid(large, BRIDGE_STATE_LIMIT));
    large[BRIDGE_STATE_LIMIT] = '}';
    CHECK(!bridge_state_valid(large, BRIDGE_STATE_LIMIT + 1));
    free(large);
    check_frame(NULL, 0, "{\"type\":\"state\",\"data\":null}\n");
    check_frame(nul, sizeof nul, "{\"type\":\"state\",\"data\":null}\n");
    check_frame((const unsigned char *)"{}", 2,
                "{\"type\":\"state\",\"data\":{}}\n");
}
