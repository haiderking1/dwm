# Reload

Super+Shift+R runs make install in the background, then replaces dwm without logging out. Configuration remains compiled C; the shortcut handles the build for you.

Edit config.h or the files under config/. config.def.h is only the template used when config.h is absent.

Reload preserves client workspace assignments, monitor assignments, tiling order, tile weights, BSP workspace trees and split ratios, focus, floating geometry, fullscreen state, selected workspaces, and monitor layouts. Existing applications keep running. Autostart runs at login, not on reload, so reload does not duplicate applications.

The bar shows build progress or failure. Build output goes to reload/build.log. A failed build leaves the running dwm unchanged. Repeated keypresses while a build runs do not start extra builds.

Paths are in config/reload.h. A running V2 binary can reload into V3 without closing applications or losing workspace assignments.

Checkpoints use V3. Monitor and client records retain their V1 byte layout, followed by the V2 float weight array and a self-delimiting BSP forest. Trees retain each monitor and workspace mask, branch axes, split ratios, leaf order, and workspace focus.

Restore accepts V1 and V2 checkpoints without a forest. V1 clients receive a weight of 1.0. Both legacy versions clear any old forest after restoring client state, so the current BSP layout seeds trees from the restored clients. V2 and V3 weights must be finite and within [0.001, 1000000].

V3 restore reads and validates the forest into temporary ownership before changing live monitors or clients. It applies the forest after restoring fields, lists, and focus, before arranging. Malformed or truncated forests and trailing V3 data are rejected without partial live mutation. Temporary trees are freed on failure.

Run the build-job and checkpoint tests with sh tests/reload/run.sh.
