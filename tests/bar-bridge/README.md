# Bridge tests

Run make -C tests/bar-bridge from the repository root, or make -C bar/bridge test.
The tests use temporary meminfo streams and stub XSendEvent and XSync. They
never connect to an X server, even if DISPLAY is set.

Coverage includes every tag mask, monitor INT_MAX and overflow, rejected signs
and whitespace, argument counts, ClientMessage slots and delivery masks,
MemAvailable and fallback accounting, KiB-to-GiB conversion, clamping, missing
or malformed counters, duplicate and overflowing counters, property size bounds,
raw control-byte rejection, escaped control characters, and state envelopes.

For a read-only live check after building:

~~~sh
timeout --signal=TERM 6s bar/bridge/dwm-bar-bridge --watch
~~~

This only reads the root state and /proc/meminfo. It should print initial state
and memory, plus another memory sample after five seconds. State changes can
produce additional lines. timeout normally exits 124 when its deadline expires.
Do not use view, move, or layout for live tests on a working desktop.
