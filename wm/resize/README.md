# Tiled mouse resizing

Hold Super and drag with the right mouse button to resize shared tile boundaries. Horizontal movement shifts the master/stack divider; vertical movement shifts the nearest available boundary above or below the selected tile. Both axes work together. The pointer stays where the drag began instead of jumping to a window corner.

Windows stay tiled and on their monitor. Adjacent tiles fill the space, while vertical resizing leaves the other tiles in that column unchanged. A single tile and monocle have no adjustable boundary. Floating windows and the floating layout keep the original free-resize behavior.

Each client has a column weight, initialized to 1. The layout partitions the workarea in pixels rather than applying application resize increments, which can leave gaps. It prefers a 32-pixel outer minimum, reducing that minimum when space is limited. If the workarea cannot fit even one content pixel plus borders per window, overlap is unavoidable.

Weights follow layout slots during drag-to-swap and survive Super+Shift+R. The reload reader accepts the previous checkpoint format and gives those clients equal weights on the first upgrade.

## Files

- math.h and layout.inc: pixel allocation and weighted columns.
- topology.inc: resize baselines and client/monitor validation.
- precision.inc and boundary.inc: bounded pairwise weight changes and shared boundaries.
- mouse.inc: pointer capture, event dispatch, and cleanup.

Mapping, closing, retagging, or rearranging a window during resizing cancels a stale drag rather than applying it to a changed layout.

Run sh tests/resize/run.sh for allocation, boundary, and mocked mouse-event tests. The runner also uses ASan/UBSan when available.
