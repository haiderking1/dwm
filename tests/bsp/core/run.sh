#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)
build=$(mktemp -d "${TMPDIR:-/tmp}/bsp-core.XXXXXX")
trap 'rm -rf "$build"' EXIT HUP INT TERM
flags="-std=c99 -pedantic -Wall -Wextra -Werror -g3 -O1 -fno-omit-frame-pointer"
case "${1:-sanitize}" in
    sanitize) flags="$flags -fsanitize=address,undefined -fno-pie -no-pie" ;;
    plain) ;;
    *) echo "usage: sh tests/bsp/core/run.sh [sanitize|plain]" >&2; exit 2 ;;
esac
# Intentional word splitting for compiler flags and wildcard source lists.
${CC:-cc} $flags -D_POSIX_C_SOURCE=200809L \
    "$root"/wm/bsp/core/*.c "$root"/tests/bsp/core/*.c \
    -Wl,--wrap=malloc -Wl,--wrap=calloc -o "$build/core-tests"
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 "$build/core-tests"
