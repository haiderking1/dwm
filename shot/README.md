# Screenshot

Print-key screenshots with no screenshot tool dependencies: no scrot, maim,
grim, slurp, xclip or a compositor. Everything runs inside dwm itself.

| Shortcut | Action |
| --- | --- |
| Print | Crosshair drag selects an area; a plain click takes the whole monitor under the pointer |
| Shift+Print | Screenshot the monitor under the pointer |
| Super+Shift+Print | Screenshot every monitor |

Escape or right click cancels a selection. The PNG is saved to shotdir
(config.h, ~/Pictures/Screenshots by default, created on demand) and served
on the CLIPBOARD selection as image/png, text/plain, text/uri-list, UTF8_STRING
and STRING.

## Why the clipboard survives games

Tools that fork xclip lose the clipboard when the helper dies, is killed by a
game launch script, or loses its race against the game's grabs. Here dwm owns
CLIPBOARD through its own 1x1 window and answers selection requests from the
main event loop. The owner lives exactly as long as the X connection, so a
paste works no matter what is running. Transfers larger than the maximum
request size switch to the ICCCM INCR protocol and chunk on each property
deletion.

## Games

A game that takes the keyboard with XGrabKeyboard defeats every passive grab:
the server routes all keys to the grab owner and Print would never reach dwm.
When XI2 is available dwm selects raw key events on the root window, and raw
events bypass grabs by design. The shot keys are then not core-grabbed at
all; they fire from the raw path exactly once per physical press, in games or
out of them. Modifier state comes from the pressed-key map, which is also a
grab-proof query. Servers older than XI2 2.1 fall back to core passive grabs
and lose only the in-game case.

The pointer is usually grabbed inside a game as well, and an active grab
cannot be taken from its owner. Instead of stealing it, Print switches to a
raw selection: raw button, motion and key events are the pointer input that
survives grabs, and the pointer position itself comes from XQueryPointer,
which grabs do not affect. A click starts the band, drag draws it, release
captures, Escape or right click cancels. The cursor stays whatever the game
shows and the game receives the click itself; the drawn band is the
indicator. Plain clicks without a drag still capture the monitor, and
servers without XI2 2.1 fall back to the full-monitor shot.

A fullscreen game also page-flips its own buffer, which can leave the root
pixmap stale, so a focused fullscreen client is copied from its window into
the freeze.

## Capture

Print copies the screen into a pixmap (root with IncludeInferiors, then any
fullscreen client on top) and maps one fullscreen canvas showing that freeze.
The rubber band is filled strips drawn on the canvas and erased by copying the
freeze back, so a browser or a game inside the square is not restacked or
repainted during the drag. The PNG is cropped from the freeze. XGetImage on that pixmap has no visual,
so the RGB masks are filled from the screen visual before encoding; without
that the file is the right size and solid black.

## Limits

- A game that takes XGrabKeyboard swallows every global key on X11, Print
  included. Pointer-only grabs (the common FPS case) still reach Print.
- The selected pixels are the frame at Print, not the frame at mouse release.
- Reload execs dwm, so clipboard ownership transfers to the new binary's
  connection only if the X fd survives the exec; pastes made before the
  reload always work.
- The encoder stores deflate blocks uncompressed, so a 4K PNG is about 25 MB
  until the clipboard owner releases it.

## Tests

~~~sh
sh tests/png/run.sh
sh tests/shot/run.sh
~~~

The PNG suite encodes deterministic images of assorted shapes and block
counts, checks chunk structure and checksums, then a Python verifier decodes
each file with zlib and compares every pixel. The shot suite paints outline
strips onto a grid and checks containment, overlap, perimeter coverage and
the interior hole, then converts pixmap GetImage layouts including the
zero-mask case that would otherwise save a black PNG.
