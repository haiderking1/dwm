#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
build=$(mktemp -d "${TMPDIR:-/tmp}/shot-png.XXXXXX")
trap 'rm -rf "$build"' EXIT HUP INT TERM
flags="-std=c99 -pedantic -Wall -Wextra -Werror -g3 -O1 -fno-omit-frame-pointer"
case "${1:-sanitize}" in
    sanitize) flags="$flags -fsanitize=address,undefined -fno-pie -no-pie" ;;
    plain) ;;
    *) echo "usage: sh tests/png/run.sh [sanitize|plain]" >&2; exit 2 ;;
esac
${CC:-cc} $flags -D_POSIX_C_SOURCE=200809L \
    "$root"/shot/png.c "$root"/tests/png/test_png.c -o "$build/png-tests"
"$build/png-tests" "$build"
python3 "$root"/tests/png/verify_png.py "$build"
