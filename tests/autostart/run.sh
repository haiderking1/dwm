#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
build=$(mktemp -d)
trap 'rm -rf "$build"' EXIT
trap 'exit 1' HUP INT TERM
cc -D_POSIX_C_SOURCE=200809L -std=c99 -pedantic -Wall -Wextra -Werror     startup/autostart.c tests/autostart/smoke.c -o "$build/test"
"$build/test"
