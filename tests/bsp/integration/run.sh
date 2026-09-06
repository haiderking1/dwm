#!/bin/sh
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$here/../../..
build=$(mktemp -d "${TMPDIR:-/tmp}/dwm-bsp-integration.XXXXXXXX")
trap 'rm -rf -- "$build"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM
python3 "$here/routing/extract.py" "$root" "$build/routes.inc"
cc=${CC:-cc}
set -- -I"$build" -std=c99 -Wall -Wextra -Werror -Wpedantic -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -Wvla
"$cc" "$@" -O2 "$here/runner.c" "$root"/wm/bsp/core/*.c -lm -o "$build/integration"
"$build/integration"
if [ "${SANITIZE:-1}" != 0 ]; then
	"$cc" "$@" -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all \
		"$here/runner.c" "$root"/wm/bsp/core/*.c -lm -o "$build/integration-sanitized"
	"$build/integration-sanitized"
fi
