# Input settings tests

Run from the repository root:

```sh
sh tests/input/run.sh
CFLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g" sh tests/input/run.sh
```

The runner compiles only the input module and tests with strict C99 warnings.
It removes its temporary build directory under tests/input on exit. It does
not build or install dwm, open an X display, or change live input settings.

The fake server checks property types, formats, lengths, truncated replies,
32-bit integer/float representation, two- and three-entry profiles, unavailable
flat profiles, missing atoms, pointer eligibility, and idempotent writes. Event
tests cover subscription preservation, cookie ownership, unrelated events,
hotplug, device changes, extension failures, and synchronous/asynchronous
unplug races. Failure tests intentionally print warnings to stderr.

A separate smoke executable links the module against the installed libXi and
libX11. It calls only the null-argument paths. These checks do not verify a live
X server or physical-device behavior.

## Module usage

Compile input/settings.c into the application and link with -lXi -lX11. Include
input/settings.h. Call input_setup(display, root) once during setup. Pass each
event to input_handle_event(display, &event) immediately after XNextEvent and
continue normal event dispatch. See the header for cookie ownership and the
single-threaded, single-display requirement.

Relative pointers include relative touchpads. Absolute-only devices, including
tablets with only relative scroll axes, receive no property changes. No tapping
or scrolling options are changed. If flat acceleration is unavailable, zero
libinput speed is only a neutral speed setting, not a guarantee of no driver
acceleration. Legacy properties and core pointer control provide the other
supported paths.
