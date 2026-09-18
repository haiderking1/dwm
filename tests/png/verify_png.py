#!/usr/bin/env python3
"""Decode the encoder's output with zlib and compare every pixel.

This is an independent implementation of the expected patterns: if the C
encoder writes a wrong byte, wrong checksum or wrong filter, this catches it.
"""

import os
import struct
import sys
import zlib


def parse(path):
    with open(path, "rb") as fh:
        data = fh.read()
    assert data[:8] == b"\x89PNG\r\n\x1a\n", f"{path}: bad signature"
    chunks = []
    pos = 8
    while pos < len(data):
        (length,) = struct.unpack(">I", data[pos:pos + 4])
        ctype = data[pos + 4:pos + 8]
        cdata = data[pos + 8:pos + 8 + length]
        (crc,) = struct.unpack(">I", data[pos + 8 + length:pos + 12 + length])
        assert crc == zlib.crc32(ctype + cdata) & 0xFFFFFFFF, f"{path}: bad CRC in {ctype}"
        chunks.append((ctype, cdata))
        pos += 12 + length
    assert pos == len(data), f"{path}: trailing bytes"
    assert [t for t, _ in chunks] == [b"IHDR", b"IDAT", b"IEND"], f"{path}: chunk sequence"
    assert chunks[2][1] == b"", f"{path}: IEND not empty"
    return chunks


def pixels(chunks):
    (ihdr,) = [c for t, c in chunks if t == b"IHDR"]
    w, h, depth, ctype, comp, filt, inter = struct.unpack(">IIBBBBB", ihdr)
    assert depth == 8 and ctype == 2 and comp == 0 and filt == 0 and inter == 0
    raw = zlib.decompress(b"".join(c for t, c in chunks if t == b"IDAT"))
    assert len(raw) == h * (1 + w * 3), f"raw size {len(raw)} for {w}x{h}"
    out = bytearray()
    for y in range(h):
        row = raw[y * (1 + w * 3):(y + 1) * (1 + w * 3)]
        assert row[0] == 0, "nonzero filter byte"
        out += row[1:]
    return w, h, bytes(out)


def lcg_pixels(n):
    data = bytearray()
    s = 0x12345678
    for _ in range(n):
        s = (s * 1103515245 + 12345) % 2147483648
        data += bytes((s & 0xFF, (s >> 8) & 0xFF, (s >> 16) & 0xFF))
    return bytes(data)


CASES = {
    "one.png": (1, 1, bytes((0xFF, 0x7F, 0x00))),
    "solid.png": (3, 2, bytes((0x10, 0x20, 0x30)) * 6),
    "odd.png": (7, 5, lcg_pixels(35)),
    "sparse.png": (1, 300, lcg_pixels(300)),
    "multi.png": (128, 256, lcg_pixels(128 * 256)),
    "wide.png": (65535, 2, lcg_pixels(65535 * 2)),
    "big.png": (513, 513, lcg_pixels(513 * 513)),
}


def main():
    directory = sys.argv[1]
    for name, (w, h, expected) in CASES.items():
        path = os.path.join(directory, name)
        cw, ch, got = pixels(parse(path))
        assert (cw, ch) == (w, h), f"{name}: {cw}x{ch} != {w}x{h}"
        assert got == expected, f"{name}: pixels differ"
        print(f"{name}: {w}x{h} ok")
    print("PNG decoder verification passed")


if __name__ == "__main__":
    main()
