# Persistent BSP dwindle

Dwindle is the default layout. Each new tiled window splits the previously focused tile along its longer dimension. The new window occupies the right or bottom child. Closing a window removes that leaf and lets its sibling occupy the parent's space. Surviving branches keep their split directions and ratios.

The layout stores a separate tree for each monitor and selected tag mask. Single-tag workspaces retain independent trees. Selecting several tags together uses its own view tree, without overwriting the single-tag arrangements. Dormant views keep their trees and prune stale clients before use.

## Controls

| Shortcut | Action |
| --- | --- |
| Super+t | BSP dwindle |
| Super+Shift+t | Weighted master-and-stack layout |
| Super+s | Rotate the focused tile's parent split |
| Super+left-drag | Swap tile identities at the drop position; empty drops return |
| Super+right-drag | Resize the nearest shared boundaries touching the tile |
| Super+h / l | Adjust the nearest horizontal ancestor split |
| Super+Return | Exchange the focused window with the first spatial leaf |
| Super+m | Monocle |
| Super+Shift+f | Floating layout |

Floating windows retain free movement and resizing. A fullscreen window that was tiled keeps its tree slot while covering the monitor. Cross-monitor drag swaps transfer the clients without rebuilding the saved slot geometry.

The layout allocates exact pixel rectangles. It prefers a 32-pixel minimum per leaf and reduces that minimum when the tree cannot fit it. Native windows still require one content pixel plus their borders, so an impossibly crowded layout can overlap. If tree allocation fails or an engine limit is reached, the current arrangement falls back to weighted tiling.

## Persistence and implementation

Super+Shift+R preserves tree topology, ratios, leaf identities and focus through a V3 reload checkpoint. V1 and V2 checkpoints remain readable. The first upgrade seeds trees from the restored client order.

- bsp.h defines the pure engine interface.
- core/ implements tree edits, geometry, boundary resizing and validated serialization.
- integration/ connects client lifecycle, focus, mouse actions and checkpoints to dwm.
- ../drag/bsp.inc keeps window-list swaps and tree slots consistent.

There is no bspwm or Wayland dependency. The engine uses libc; the existing dwm integration uses Xlib.

## Tests

~~~sh
sh tests/bsp/core/run.sh
sh tests/bsp/integration/run.sh
sh tests/reload/run.sh
sh tests/drag/run.sh
sh tests/resize/run.sh
~~~

The suites cover randomized tree edits, malformed checkpoints, stale resize snapshots and mocked X11 events. They do not replace physical multi-monitor testing.
