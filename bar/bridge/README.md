# dwm-bar-bridge

Native C99 bridge for the dwm Quickshell root-property protocol. The executable
uses Xlib and standard/POSIX APIs. It does not start commands or other runtimes.

## Build and test

From /home/soka/dwm:

~~~sh
make -C bar/bridge
make -C bar/bridge test
~~~

The output is /home/soka/dwm/bar/bridge/dwm-bar-bridge. The build needs a C99
compiler, make, and the existing Xlib headers and library. It links with -lX11.
Tests do not open an X display or change properties, tags, or windows.

## Watch

~~~sh
bar/bridge/dwm-bar-bridge --watch
~~~

The watcher subscribes to root PropertyChangeMask before its first read. It
immediately prints a state envelope and a memory sample, then prints state on
_DWM_QUICKSHELL_STATE changes and memory every five seconds. Each record is one
JSON line, flushed to stdout. SIGINT and SIGTERM stop the watch.

~~~json
{"type":"state","data":null}
{"type":"memory","percent":25.000000,"usedGiB":4.000000}
~~~

State data is the root JSON object without reserialization. Missing properties,
wrong types or formats, values exceeding 1 MiB, and invalid framing produce
null. The property must be UTF8_STRING, format 8, start with an opening brace,
and end with a closing brace. Raw control bytes, including NUL, LF, and CR, are
rejected. Escaped JSON control characters are preserved. This is a framing guard,
not a JSON or UTF-8 grammar validator. The dwm property writer supplies valid JSON.

The watcher reads /proc/meminfo directly. It uses MemAvailable or, when absent,
MemFree + Buffers + Cached + SReclaimable - Shmem. Availability is clamped to
0..MemTotal. Percent is used / total * 100; usedGiB is used KiB / 1048576.
Invalid or unreadable samples produce a stderr diagnostic and retry after five
seconds without interrupting state updates. A monotonic poll deadline and bounded
128-event batches keep the timer active during X event storms. State changes
within one batch are coalesced to the latest value.

## Commands

~~~text
dwm-bar-bridge view MONITOR TAG
dwm-bar-bridge move MONITOR TAG
dwm-bar-bridge layout MONITOR
~~~

MONITOR is decimal 0..INT_MAX; TAG is decimal 1..10. Signs, whitespace, trailing
characters, and extra arguments are rejected before connecting to X. Leading
zeroes are decimal. Invalid arguments exit 2; connection and I/O failures exit 1.

Commands send a format-32 root ClientMessage named _DWM_QUICKSHELL_COMMAND.
The five long slots hold operation, monitor, argument, 0, 0. Operation 1 is view,
2 is move, and 3 is layout. View and move arguments are 1U << (TAG - 1); layout
uses 0. Delivery uses SubstructureRedirectMask | SubstructureNotifyMask and
XSync. Successful delivery does not acknowledge that dwm acted on the command.

## Parent Makefile integration

This directory is self-contained. No root Makefile changes or installation are
part of this implementation. The parent build can invoke these recipes later:

~~~make
# In the parent's build recipe:
	$(MAKE) -C bar/bridge
# In the parent's test recipe:
	$(MAKE) -C bar/bridge test
# In the parent's install recipe, after integration:
	$(MAKE) -C bar/bridge install PREFIX="$(PREFIX)" DESTDIR="$(DESTDIR)"
# In the parent's clean recipe:
	$(MAKE) -C bar/bridge clean
~~~

Do not include this Makefile directly; its paths are relative to bar/bridge.
The standalone install defaults to PREFIX=/home/soka/.local, supports DESTDIR,
and stages a mode-755 file in the destination bin directory before an atomic
rename. It never truncates a running executable. Normal builds and tests do not
install anything.
