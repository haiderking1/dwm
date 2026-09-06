#!/bin/sh
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
work=$(mktemp -d /tmp/dwm-reload-tests.XXXXXX)
trap 'rm -rf "$work"' EXIT HUP INT TERM
"${CC:-cc}" -std=c99 -Wall -Wextra -Wpedantic -Werror -O2 \
    "$here/../../reload/build.c" "$here/fixture.c" "$here/test_build.c" \
    -o "$work/test-build"
"$work/test-build" "$work/test-build"

for test in test_state.c test_weights.c forest/test_forest.c; do
    name=$(basename "$test" .c)
    "${CC:-cc}" -D_POSIX_C_SOURCE=200809L -std=c99 -pedantic -Wall -Wextra -Werror \
        ${CFLAGS:-} "$here/$test" "$here"/../../wm/bsp/core/*.c \
        ${LDFLAGS:-} -lm -o "$work/$name"
    "$work/$name"
done
