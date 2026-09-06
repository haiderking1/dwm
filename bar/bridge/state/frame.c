#include "state.h"

int
bridge_state_valid(const unsigned char *data, size_t length)
{
    size_t i;
    if (!data || length < 2 || length > BRIDGE_STATE_LIMIT ||
        data[0] != '{' || data[length - 1] != '}')
        return 0;
    for (i = 0; i < length; ++i)
        if (data[i] < 0x20)
            return 0;
    return 1;
}

int
bridge_write_state(FILE *output, const unsigned char *data, size_t length)
{
    if (fputs("{\"type\":\"state\",\"data\":", output) == EOF)
        return 0;
    if (bridge_state_valid(data, length)) {
        if (fwrite(data, 1, length, output) != length)
            return 0;
    } else if (fputs("null", output) == EOF) {
        return 0;
    }
    return fputs("}\n", output) != EOF && fflush(output) == 0;
}
