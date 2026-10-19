"""Converts the P4 PBM screens in a directory to scaled-up PNGs in an LCD-like
palette, plus sheet.png with all screens of that directory side by side."""
import pathlib
import struct
import sys
import zlib

SCALE = 3
COLUMNS = 4
GAP = 8
ON, OFF, BORDER = (230, 240, 255), (16, 20, 28), (90, 90, 90)


def write_png(path, rows):
    height, width = len(rows), len(rows[0]) // 3
    raw = b"".join(b"\0" + bytes(row) for row in rows)

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))

    path.write_bytes(b"\x89PNG\r\n\x1a\n"
                     + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
                     + chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b""))


def read_pbm(path):
    data = path.read_bytes()
    magic, dims, rest = data.split(b"\n", 2)
    assert magic == b"P4", path
    width, height = map(int, dims.split())
    stride = (width + 7) // 8
    rows = []
    for y in range(height):
        bits = rest[y * stride:(y + 1) * stride]
        row = bytearray()
        for x in range(width):
            row += bytes(ON if bits[x // 8] & (0x80 >> (x % 8)) else OFF) * SCALE
        rows.extend([row] * SCALE)
    return rows


screens = []
for pbm in sorted(pathlib.Path(sys.argv[1]).glob("*.pbm")):
    rows = read_pbm(pbm)
    write_png(pbm.with_suffix(".png"), rows)
    screens.append(rows)

if screens:
    h, w = len(screens[0]), len(screens[0][0])
    cols = min(COLUMNS, len(screens))
    nrows = (len(screens) + cols - 1) // cols
    gap = GAP * 3
    sheet = [bytearray(bytes(BORDER) * ((cols * w + (cols - 1) * gap) // 3)) for _ in range(nrows * h + (nrows - 1) * GAP)]
    for n, rows in enumerate(screens):
        ox, oy = (n % cols) * (w + gap), (n // cols) * (h + GAP)
        for y, row in enumerate(rows):
            sheet[oy + y][ox:ox + w] = row
    write_png(pathlib.Path(sys.argv[1]) / "sheet.png", sheet)
