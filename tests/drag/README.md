# Drag placement tests

Run `./tests/drag/run.sh` from the repository root after
`wm/drag/placement.inc` exists. The runner compiles that production include
against minimal fixtures. It creates and removes a temporary build directory.
Missing production code returns exit status 77 without building.

`CC=clang` selects a compiler. ASan and UBSan run when a compiler and runtime
probe succeeds. Use `SANITIZE=0` to disable them or `SANITIZE=1` to require
them. A failing sanitized test is always a failure, not a skipped test.

Coverage includes border-inclusive logical geometry with half-open edges,
stack precedence and occlusion, source and visibility exclusions, and the
active layout and tagset. Swap tests enumerate ordered same-monitor pairs
and cross-monitor client and stack positions, including single-node lists.
Selections cover exchanged clients, other clients and NULL. Exact expected
lists catch cycles and lost nodes. Whole-fixture comparisons check unchanged
tags, focus order and unrelated fields, and reject partial changes on failure.
