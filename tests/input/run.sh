#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
build=$(mktemp -d tests/input/.build.XXXXXX)
trap 'rm -rf "$build"' EXIT
trap 'exit 1' HUP INT TERM

# CFLAGS is deliberately word-split for optional sanitizer/compiler flags.
${CC:-cc} -std=c99 -pedantic -Wall -Wextra -Werror ${CFLAGS:-} \
    input/settings.c tests/input/fake_xlib.c tests/input/fake_xi.c \
    tests/input/properties_test.c tests/input/events_test.c tests/input/main.c \
    -o "$build/settings-test"
"$build/settings-test"

${CC:-cc} -std=c99 -pedantic -Wall -Wextra -Werror ${CFLAGS:-} \
    input/settings.c tests/input/link_test.c -lXi -lX11 -o "$build/link-test"
"$build/link-test"
