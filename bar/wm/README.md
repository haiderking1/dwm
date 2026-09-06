# Quickshell X11 integration

Set `externalbar = 1` in `config.h` (the default in both config files).
Native drawing and native space reservation are disabled. Each monitor retains
an unmapped compatibility window because `reload/restore.inc` moves it directly.
`updatebarpos()` clears restored `showbar` before that move. Nothing maps the
compatibility window in external mode. Set `externalbar = 0` and rebuild to use
the native fallback.

## State

Root property: `_DWM_QUICKSHELL_STATE`. Type: `UTF8_STRING`. Format: 8.
The value is JSON without a trailing NUL, with exactly these keys:

```json
{"tags":["1","2","3","4","5","6","7","8","9","10"],"selectedMonitor":0,"monitors":[{"num":0,"x":0,"y":0,"width":1920,"height":1080,"selected":1,"occupied":3,"urgent":0,"layout":"[]=","title":"escaped title","fullscreen":false}]}
```

- `num` is dwm's monitor number. Match panels by monitor geometry, not array order.
- Geometry is the full monitor rectangle, not the strut-reduced work area.
- `selectedMonitor` identifies the globally selected monitor.
- `selected`, `occupied` and `urgent` are tag bit masks; bit 0 is tag 1.
- `layout` is the current layout symbol, including monocle's client count.
- `title` is the monitor's selected visible client's title, or an empty string.
- `fullscreen` means that monitor's selected visible client is fullscreen.
  It is independent of which monitor is globally selected. A fullscreen client
  on a hidden tag does not set it.
- Strings are JSON-escaped. Invalid/truncated UTF-8 bytes become U+FFFD.

Read the property initially, then on root PropertyNotify. dwm publishes after
scan, after restore/startup, and once per bounded event batch. Equal JSON is
not written again, so property notifications cannot sustain a feedback loop.

## Commands

Send an X ClientMessage to the root using
`SubstructureRedirectMask | SubstructureNotifyMask`:

- message_type: `_DWM_QUICKSHELL_COMMAND`
- format: `32`
- data.l: `[operation, monitorNumber, argument, 0, 0]`

| Operation | Argument | Effect |
| --- | --- | --- |
| 1 | Tag bit mask | View those tags on the target monitor |
| 2 | Tag bit mask | Retag the target monitor's selected client |
| 3 | Ignored | Cycle forward through configured layouts |
| 4 | Ignored | Native togglebar; no effect in external mode |

All operations require an existing monitor. View/tag mask arguments with
TAGMASK and reject a zero result. Accepted operations select the target monitor,
except the inert external toggle. Tag is a no-op without a selected client.
Invalid format, operation or monitor is ignored. No shell execution or eval is
involved. There is no command acknowledgement; observe state updates.

## Panel contract

Use an XPanelWindow with `_NET_WM_WINDOW_TYPE_DOCK` and a CARDINAL/32
`_NET_WM_STRUT_PARTIAL` containing all 12 EWMH values. Coordinates and depths
are root-relative; span endpoints are inclusive. A valid partial strut wins
over `_NET_WM_STRUT`. The four-value legacy property reserves full-edge spans.
No strut means no reserved space. Reservations only apply while mapped.
All four edges are intersected with each monitor, and overlapping reservations
use the maximum depth rather than adding heights.

Normal and override-redirect docks are discovered during scan and through root
creation/map/property events. They never enter tiling, focus selection, or
`_NET_CLIENT_LIST`. Map, unmap, destroy, reparent and strut/type changes refresh
work areas. Fullscreen clients still use the full monitor rectangle.

The QML worker must bind each panel's `aboveWindows` to the inverse of that
monitor's `fullscreen` field. dwm does not forcibly unmap Quickshell panels.
The bridge must consume the root property and send the messages above; no
reload or autostart changes are required by this implementation.

## Validation

`make -j2` builds dwm. `python3 bar/wm/tests/run.py` runs geometry, JSON and
real-handler integration tests against Xlib mocks. Set `SANITIZE=1` for
AddressSanitizer/UndefinedBehaviorSanitizer. Tests never open a display or call
dwm's session entry point. Actual QML stacking and hotplug behavior still need
a later live-session check authorized by the user.
