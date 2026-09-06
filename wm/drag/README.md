# Tiled drag-to-swap

Hold Super and drag a tiled window with the left mouse button. Release over another visible tiled window to exchange their layout slots. Release over the bar, empty space, or a floating/fullscreen window to return to the original slot.

The dragged window follows the pointer, but its logical geometry and client-list position remain unchanged until release. Other tiles do not collapse into its slot while dragging. Drops use the pointer position, not the dragged window's center. Movement under four pixels counts as a click.

Cross-monitor drops exchange the two windows' monitor and tag assignments with their slots. Same-monitor swaps leave tag assignments intact. Floating windows and the floating layout keep dwm's normal free dragging; fullscreen windows cannot be dragged.

placement.inc contains targeting and list operations. mouse.inc owns pointer capture, preview movement, event handling, and final restoration. It looks clients up again after dispatching events so closing a window during a drag cannot leave a stale pointer.

Run sh tests/drag/run.sh for placement and mocked X11 event-loop tests, including ASan/UBSan when available.
