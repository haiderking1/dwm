# Rubber-band geometry and freeze-pixmap RGB tests

Paints every outline that `shot_band_edges` can emit for a range of origins,
sizes and thicknesses onto a grid. A case fails if a strip leaves the
rectangle, two strips share a pixel, the perimeter has a gap, or the interior
hole is painted.

The RGB suite feeds the pixmap GetImage layout Print actually crops: 24- and
32-bpp ZPixmap rows, including the zero-mask case that encodes as a black PNG
until the screen visual fills those masks.

~~~sh
sh tests/shot/run.sh            # address/UB sanitized
sh tests/shot/run.sh plain      # no sanitizers
~~~
