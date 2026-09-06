# Mouse drag event-loop tests

Run `SANITIZE=1 ./tests/drag/run.sh` from the repository root to require
ASan and UBSan. The shared runner also runs the existing placement tests.
`CC=clang` selects another compiler; `SANITIZE=0` runs without sanitizers.

`runner.c` includes the real `wm/drag/placement.inc` and
`wm/drag/mouse.inc`. The fixture uses `<X11/Xlib.h>` and defines the Xlib
functions those includes call. It needs Xlib headers, but neither an X
server nor linking against libX11.

The server-window positions are separate from the clients' logical
rectangles. The mocked layout sends geometry requests only when a logical
rectangle changes. Empty and bar drops must therefore restore the server
position with an explicit `XMoveWindow`; an arrange call alone cannot pass.
Queue checkpoints inspect client geometry, floating state, list links and
selection while the preview is visible. Final checks inspect exact client
order, geometry, tags, focus, server positions and restoration requests.

The 16 test groups cover occupied and cross-monitor swaps, empty and bar
drops, release coordinates without motion or different from the last
motion, the four-pixel threshold, failed grabs, failed pointer lookup,
source and target destruction, other mouse buttons, throttled motion,
configure requests, changed monitor geometry, dock workarea updates,
layout cancellation and target eligibility changes. Destroy handlers
unlink and actually free clients, so sanitizer runs check for stale
pointers across event dispatch.

The event queue has a fixed capacity and fails immediately on exhaustion.
Enter-event draining and recorded X moves are also bounded. Each group is
printed before it runs so an assertion failure identifies its scenario.

The dwm callbacks are mocks, not integration coverage of dwm's callback
implementations. ConfigureRequest acknowledges tiled geometry without
applying it. ConfigureNotify updates the fixture's tile rectangles and
arranges them. PropertyNotify models a dock workarea change, a switch to a
non-arranging layout, or a target becoming fullscreen. These events test
dispatch and revalidation in the production drag loop.
