# BSP core tests

Run from the repository root:

```sh
sh tests/bsp/core/run.sh
sh tests/bsp/core/run.sh plain
```

The first command enables ASan, UBSan, and leak detection. Both modes compile
all `wm/bsp/core/*.c` with C99, warnings as errors, and no X11 or math library.
The runner uses a temporary build directory and removes it on exit. It does not
change the main build or enable BSP in dwm. Set `CC` to select a compiler.
Allocation-failure tests require GNU-compatible linker `--wrap` support.
The pipe test uses POSIX APIs; the engine itself uses only libc.

## Coverage

- Membership deletion, parent collapse, stable IDs and ratios, reordered input,
  explicit anchors, focus fallback, and insertion using the new bounds.
- Monitor and tag-mask isolation, repeated IDs across views, global forgetting,
  focus, slot swaps, replacement, rotation, and nearest-axis adjustment.
- Exact pixel coverage, uneven ratios, subtree minimums, nearest touching edges,
  protected siblings, two-axis release deltas, repeated updates, and stale tokens.
- Empty and populated persistence round trips, concatenated frames through a
  pipe, every byte truncation of a three-view fixture, malformed fields,
  duplicate identities, duplicate views, invalid preorder trees, NaN/infinity,
  depth overflow, and 4,096 deterministic structured mutations.
- Maximum-size and maximum-depth trees, ID exhaustion, 1,024 views, extreme
  coordinates, and impossible minimum sizes.
- 20,000 deterministic mixed operations across four views, checking membership,
  parent links, IDs, geometry, ratios, area, and focus after every operation.
  This includes 80 persistence replacements.
- Allocation failures at every allocation position in reconciliation and a
  populated read, plus registry creation and writer validation failures.

## Limits and behavior

A tree permits 4,096 leaves, 8,191 total nodes, and 128 parent edges from root to
leaf. Splitting a leaf at depth 128 fails without rebalancing. A batch sync can
therefore reach the depth limit before the leaf limit. Registry keys require a
nonnegative monitor and a nonzero 32-bit tag mask. A registry permits 1,024 views.
Persistence permits 131,071 nodes across the forest and at most 4,227,064 bytes
per frame. A live registry can exceed the persistence node budget; writing it
then fails before emitting a header.

Stored split ratios stay within 0.05 through 0.95. Layout uses a 32-pixel default
leaf minimum, or the positive minimum supplied by the caller. It sums subtree
requirements along a split axis and takes their maximum across the other axis.
When the requested minimum cannot fit, layout divides the available pixels in
proportion to subtree requirements. Zero-sized leaves are allowed when needed.
Otherwise it clamps the rounded ratio cut to both subtree minimums. Layout does
not replace the stored ratio with that clamped cut.

Negative dimensions become zero. Positive dimensions are clipped so that right
and bottom coordinates remain representable as `int`. Negative origins work.
Equal width and height choose a left/right split. Existing leaves remain first
children; new leaves become second children. Focus stays on its window identity
when slots swap. Removing focus chooses the first surviving leaf.

Generation tokens are process-wide and are not serialized. Topology, focus,
slot identity, rotation, bounds, and minimum changes invalidate resize baselines.
Ratio updates do not, so a baseline supports repeated release updates. Baselines
use the visible pixel boundary and the original pointer coordinates, not cached
node pointers. Each axis selects the touching ancestor boundary nearest the
initial pointer; equal distances favor the nearer ancestor. At generation-token
exhaustion, new tokens saturate and resizing those trees is disabled. Node IDs
never wrap; insertion fails when it cannot reserve fresh IDs.

Invalid membership input leaves the tree unchanged. Allocation or depth failure
can leave a partially reconciled tree, with valid ownership, geometry, focus,
and generation. Failed reads leave the output forest unchanged. Successful reads
replace it and free its previous contents. Callers own forests, must serialize
access, and must not edit node links or mutate a tree from its visit callback.

The private binary format is described in [format.md](format.md).
