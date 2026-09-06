#!/bin/sh
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
work=$(mktemp -d /tmp/dwm-reload-tests.XXXXXX)
trap 'rm -rf "$work"' EXIT HUP INT TERM
"${CC:-cc}" -std=c99 -Wall -Wextra -Wpedantic -Werror -O2 \
    "$here/../../reload/build.c" "$here/fixture.c" "$here/test_build.c" \
    -o "$work/test-build"
"$work/test-build" "$work/test-build"

"${CC:-cc}" -D_POSIX_C_SOURCE=200809L -std=c99 -pedantic -Wall -Wextra -Werror \
    "$here/test_state.c" -o "$work/test-state"
"$work/test-state"
