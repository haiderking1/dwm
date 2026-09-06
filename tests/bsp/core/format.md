# Private forest format, version 1

All integers use unsigned little-endian encoding. Ratios use IEEE-754 binary64
bits in little-endian order. The implementation requires binary64 `double`.
No pointers, rectangles, generation tokens, or layout caches are stored.

## Header, 24 bytes

| Offset | Bytes | Value |
| --- | --- | --- |
| 0 | 8 | ASCII `BSPFRST` followed by a zero byte |
| 8 | 4 | Version, exactly 1 |
| 12 | 4 | Workspace count |
| 16 | 4 | Total node count across all workspaces |
| 20 | 4 | Reserved, zero |

Each workspace record is followed immediately by its declared node records.

## Workspace, 32 bytes

| Offset | Bytes | Value |
| --- | --- | --- |
| 0 | 4 | Monitor, at most INT_MAX |
| 4 | 4 | Nonzero tag mask |
| 8 | 4 | Node count, zero or an odd number at most 8,191 |
| 12 | 4 | Preferred minimum, 1 through INT_MAX |
| 16 | 8 | Focused window, zero only for an empty tree |
| 24 | 8 | Next node ID, nonzero and greater than every node ID |

Monitor/tag pairs must be unique. Window IDs may recur in different workspaces.

## Node, 32 bytes

| Offset | Bytes | Value |
| --- | --- | --- |
| 0 | 8 | Unique nonzero node ID within this tree |
| 8 | 8 | Nonzero window for a leaf, zero for a branch |
| 16 | 8 | Finite ratio, 0.05 through 0.95 inclusive |
| 24 | 1 | Axis, 0 for left/right or 1 for top/bottom |
| 25 | 1 | Kind, 0 for branch or 1 for leaf |
| 26 | 6 | Reserved, all zero |

Nodes use preorder. Every branch has exactly two following subtrees, first child
before second child; a leaf has none. Node counts must match this traversal
exactly. Root depth is zero and the maximum depth is 128. Leaf window IDs must
be unique within each tree, and focus must name one of them. Axes and ratios
must be valid even in leaf records.

## Stream and failure rules

A frame occupies exactly `24 + 32 * workspaces + 32 * nodes` bytes. The reader
consumes one frame and leaves following bytes available through the same
`FILE *`. It does not seek, require EOF, or reject a following frame. Standard
stdio buffering may read ahead from an underlying descriptor; continue through
the same stream rather than reading that descriptor directly.

The reader validates into temporary ownership and commits only after all records
pass. Failure frees the temporary forest and leaves the output unchanged; it
does not promise to restore the input stream position. The writer validates the
whole forest before emitting bytes. An I/O failure can still leave partial
output. The caller handles flushing, closing, and durable file replacement.
