#!/usr/bin/env python3
"""Compile and run offline tests; never open an X display or invoke dwm main."""
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
CC = shlex.split(os.environ.get("CC", "cc"))
FLAGS = ["-std=c99", "-Wall", "-Wextra", "-Wno-unused-parameter",
         "-Wno-deprecated-declarations", "-D_DEFAULT_SOURCE", "-D_XOPEN_SOURCE=700",
         '-DVERSION="test"', "-DXINERAMA", "-g", "-O1"]
if os.environ.get("SANITIZE"):
    FLAGS += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]


def run(args):
    return subprocess.check_output(args, cwd=ROOT, text=True)


with tempfile.TemporaryDirectory(prefix="dwm-qs-tests-") as directory:
    directory = Path(directory)
    def build(name, sources, extra=()):
        target = directory / name
        subprocess.run(CC + FLAGS + sources + list(extra) + ["-o", str(target)],
                       cwd=ROOT, check=True)
        return str(target)

    geometry = build("geometry", ["bar/wm/tests/geometry.c", "bar/wm/geometry.c"])
    run([geometry])
    encoder = build("json", ["bar/wm/tests/json.c", "bar/wm/json.c"])
    encoded = [json.loads(line) for line in run([encoder]).splitlines()]
    assert encoded == ["".join(map(chr, [34, 92, 10, 9, 13, 1])),
                       "日本語 😀", "�" * 2, "�" * 3, ""]
    xflags = shlex.split(run(["pkg-config", "--cflags", "--libs", "x11", "xi",
                             "xinerama", "xft", "fontconfig"]))
    integration = build("integration", ["bar/wm/tests/integration.c", "bar/wm/tests/xmock.c",
                        "bar/wm/geometry.c", "bar/wm/json.c", "drw.c", "util.c",
                        "input/settings.c", "startup/autostart.c", "reload/build.c"], xflags)
    states = [json.loads(line) for line in run([integration]).splitlines()]
    expected = {"tags": [str(i) for i in range(1, 11)], "selectedMonitor": 0,
                "monitors": [{"num": 0, "x": 0, "y": 0, "width": 1920, "height": 1080,
                              "selected": 1, "occupied": 3, "urgent": 3, "layout": "[]=",
                              "title": "title " + "".join(map(chr, [34, 92, 10])),
                              "fullscreen": False}]}
    assert states[0] == expected, states[0]
    expected["monitors"][0]["fullscreen"] = True
    assert states[1] == expected
    assert states[2]["monitors"][0]["fullscreen"] is False
    assert states[2]["monitors"][0]["title"] == ""
    assert states[2]["monitors"][0]["selected"] == 4
    assert len(states[3]["monitors"]) == 2
    assert states[3]["monitors"][1]["num"] == 1
    assert states[3]["monitors"][1]["x"] == 1920
print("PASS: strut geometry, UTF-8 JSON, dock lifecycle, checkpoint bar guard, state cache, commands")
