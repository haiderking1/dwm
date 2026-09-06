#!/bin/sh
set -eu

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
if [ ! -f "$here/../../wm/drag/placement.inc" ] || [ ! -f "$here/../../wm/drag/mouse.inc" ]; then
	printf '%s\n' 'drag: production includes are not available; tests not run' >&2
	exit 77
fi

cc=${CC:-cc}
sanitize=${SANITIZE:-auto}
case "$sanitize" in
	0|1|auto) ;;
	*) printf '%s\n' 'SANITIZE must be 0, 1, or auto' >&2; exit 2 ;;
esac
build=$(mktemp -d "${TMPDIR:-/tmp}/dwm-drag-tests.XXXXXXXX")
trap 'rm -rf -- "$build"' EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

set -- -std=c99 -Wall -Wextra -Werror -Wpedantic -Wshadow \
	-Wstrict-prototypes -Wmissing-prototypes -Wformat=2 -Wundef -Wvla
"$cc" "$@" -O2 "$here/runner.c" -o "$build/placement"
"$build/placement"
"$cc" "$@" -O2 "$here/mouse/runner.c" -o "$build/mouse"
"$build/mouse"

if [ "$sanitize" != 0 ]; then
	printf '%s\n' 'int main(void) { return 0; }' > "$build/probe.c"
	if "$cc" "$@" -O1 -g -fno-omit-frame-pointer \
		-fsanitize=address,undefined -fno-sanitize-recover=all \
		"$build/probe.c" -o "$build/probe" > "$build/probe.log" 2>&1 \
		&& "$build/probe" >> "$build/probe.log" 2>&1; then
		"$cc" "$@" -O1 -g -fno-omit-frame-pointer \
			-fsanitize=address,undefined -fno-sanitize-recover=all \
			"$here/runner.c" -o "$build/placement-sanitized"
		"$build/placement-sanitized"
		"$cc" "$@" -O1 -g -fno-omit-frame-pointer \
			-fsanitize=address,undefined -fno-sanitize-recover=all \
			"$here/mouse/runner.c" -o "$build/mouse-sanitized"
		"$build/mouse-sanitized"
	else
		printf '%s\n' 'drag: ASan/UBSan unavailable' >&2
		if [ "$sanitize" = 1 ]; then
			cat "$build/probe.log" >&2
			exit 1
		fi
	fi
fi
