# BSP integration tests

Run from the repository root:

	sh tests/bsp/integration/run.sh

The runner needs a C99 compiler, Python 3, and Xlib headers, but no X server. It runs optimized and ASan/UBSan builds by default. Set SANITIZE=0 to run only the optimized build.

The harness links the real engine and includes the production forest, layout, keyboard, checkpoint, drag adapter, and mouse loops. Xlib calls use checked mocks. A build-time extractor also compiles dwm's actual resizemouse, setmfact, and zoom functions; routing spies reject floating resize, pointer warp, and monitor migration.

Coverage:

- Deterministic seeding from client order, cached-focus insertion, persistent full-mask views, stale membership pruning, and fullscreen slot retention without resizing fullscreen clients.
- Parent rotation, nearest horizontal ratio adjustment, and zoom's spatial leaf swap.
- Checkpoint ratios and focus, detached read ownership, invalid tag masks, missing clients and monitors, and temporary ownership transfer.
- Same-monitor drag leaf swaps, mixed BSP/weighted monitor swaps, all saved-view identity replacement, and stale identities left by earlier migration.
- Release-only resizing, throttled final motion, unrelated buttons, failed grabs and pointer queries, source destruction/unmapping, insertion, neighbor removal, view/layout/bounds/monitor changes, and snapshot invalidation after focus changes.
- Default layout indices and keyboard bindings; weighted routing and monocle's resize no-op.

The older suites remain separate:

	sh tests/drag/run.sh
	sh tests/resize/run.sh

These tests do not establish live X-server behavior, physical monitor hotplug behavior, or successful process replacement. They do not launch dwm, install binaries, or invoke reload.
