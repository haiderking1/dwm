# PNG encoder tests

Encodes deterministic images that cover a single pixel, odd widths, a
one-pixel-wide column, and raw sizes crossing one and many zlib stored-block
boundaries. The C side checks chunk structure, zlib framing and IEND, then a
Python verifier parses every chunk, validates CRCs, decompresses the IDAT
with zlib and compares all pixels against an independently generated pattern.

~~~sh
sh tests/png/run.sh            # address/UB sanitized
sh tests/png/run.sh plain      # no sanitizers
~~~
