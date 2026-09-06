"""Compile selected dwm entry points verbatim, not a second routing implementation."""
from pathlib import Path
import sys

root, destination = map(Path, sys.argv[1:])
source = (root / "dwm.c").read_text()
functions = []
for name in ("resizemouse", "setmfact", "zoom"):
    start = source.index("\n" + name + "(const Arg *arg)\n{")
    end = source.index("\n}\n", start) + 3
    functions.append("static void" + source[start:end])
destination.write_text("\n".join(functions))

# Checkpoint indices and the actual binding table are part of the integration.
for filename in ("config.h", "config.def.h"):
    config = (root / filename).read_text()
    layouts = config.split("static const Layout layouts[] = {", 1)[1].split("};", 1)[0]
    entries = [line for line in layouts.splitlines() if "{" in line]
    assert len(entries) == 4
    for entry, expected in zip(entries, ("dwindle", "NULL", "monocle", "tile")):
        assert expected in entry, (filename, entry)
keys = (root / "config/keys.h").read_text()
assert "XK_s,      bsp_rotate_selected" in keys
assert "XK_t,      setlayout,      {.v = &layouts[0]}" in keys
assert "MODKEY|ShiftMask,             XK_t,      setlayout,      {.v = &layouts[3]}" in keys
