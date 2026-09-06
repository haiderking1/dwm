#!/bin/sh
set -eu
if [ "$(id -u)" -ne 0 ]; then
    echo "Run this script as root to install the SDDM session entry." >&2
    exit 1
fi
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
install -d -m 755 /usr/local/share/xsessions
install -m 644 "$script_dir/dwm.desktop" /usr/local/share/xsessions/dwm.desktop
