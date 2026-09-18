#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
build=$(mktemp -d "${TMPDIR:-/tmp}/shot-band.XXXXXX")
trap 'rm -rf "$build"' EXIT HUP INT TERM
flags="-std=c99 -pedantic -Wall -Wextra -Werror -g3 -O1 -fno-omit-frame-pointer"
case "${1:-sanitize}" in
    sanitize) flags="$flags -fsanitize=address,undefined -fno-pie -no-pie" ;;
    plain) ;;
    *) echo "usage: sh tests/shot/run.sh [sanitize|plain]" >&2; exit 2 ;;
esac
${CC:-cc} $flags -D_POSIX_C_SOURCE=200809L \
    "$root"/shot/band.c "$root"/tests/shot/test_band.c -o "$build/band-tests"
${CC:-cc} $flags -D_POSIX_C_SOURCE=200809L \
    "$root"/shot/rgb.c "$root"/tests/shot/test_rgb.c -o "$build/rgb-tests"
"$build/band-tests"
"$build/rgb-tests"
