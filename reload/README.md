# Reload

Super+Shift+R runs make install in the background, then replaces dwm without logging out. Configuration remains compiled C; the shortcut handles the build for you.

Edit config.h or the files under config/. config.def.h is only the template used when config.h is absent.

Reload preserves client workspace assignments, monitor assignments, tiling order, focus, floating geometry, fullscreen state, selected workspaces, and monitor layouts. Existing applications keep running. Autostart runs at login, not on reload, so reload does not duplicate applications.

The bar shows build progress or failure. Build output goes to reload/build.log. A failed build leaves the running dwm unchanged. Repeated keypresses while a build runs do not start extra builds.

Paths are in config/reload.h. The initial installation requires one logout/login because the older running binary does not have this shortcut.

Run the build-job and checkpoint tests with sh tests/reload/run.sh.
