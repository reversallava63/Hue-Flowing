#!/usr/bin/env python3
"""
png_to_rgb565.py

Converts PNG sprite/tile images into C headers of raw RGB565 pixel arrays
for use with eadk_display_push_rect() on the NumWorks N0120.

Usage:
    python3 png_to_rgb565.py input.png output.h SPRITE_NAME
    python3 png_to_rgb565.py input.png output.h SPRITE_NAME --transparent 255,0,255

Notes:
- eadk_display_push_rect(rect, pixels) expects a flat array of eadk_color_t
  (uint16_t, RGB565), row-major, top-to-bottom, left-to-right.
- If the source PNG has an alpha channel, pixels with alpha < 128 are mapped
  to a chosen "transparent" magic color (default 0x0000, black) so they
  blend with the black background.
- For sprites you plan to draw with a solid background anyway (no
  transparency needed), just ignore the alpha handling downstream.
"""

import sys
import argparse
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("This script requires Pillow: pip install pillow --break-system-packages")
    sys.exit(1)

MAGIC_TRANSPARENT = 0x0001   # was 0x0000


def rgb888_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def convert(png_path: Path, header_path: Path, sprite_name: str,
            alpha_threshold: int = 128, transparent_key=None,
            strip_corner_markers: bool = False):
    img = Image.open(png_path).convert("RGBA")
    w, h = img.size
    pixels = img.load()

    if strip_corner_markers:
        # dafluffypotato's export tool stamps a magenta pixel at (1,0) and a
        # cyan pixel at (w-1,0) on each packed tile as an alignment marker.
        # Replace them with the pixel just below so they don't show up as
        # stray colored dots when rendered.
        for (mx, my) in [(1, 0), (w - 1, 0)]:
            if 0 <= mx < w and 0 <= my < h - 1:
                pixels[mx, my] = pixels[mx, my + 1]

    out = []
    n_transparent = 0
    for y in range(h):
        row = []
        for x in range(w):
            r, g, b, a = pixels[x, y]
            if a < alpha_threshold:
                row.append(MAGIC_TRANSPARENT)
                n_transparent += 1
            else:
                if transparent_key is not None and (r, g, b) == transparent_key:
                    row.append(MAGIC_TRANSPARENT)
                    n_transparent += 1
                else:
                    row.append(rgb888_to_rgb565(r, g, b))
        out.extend(row)

    macro_base = sprite_name.upper()
    lines = []
    lines.append(f"// Auto-generated from {png_path.name} by png_to_rgb565.py")
    lines.append(f"// {w}x{h} pixels, RGB565, magic transparent key = 0x{MAGIC_TRANSPARENT:04X}")
    lines.append("#pragma once")
    lines.append("#include <stdint.h>")
    lines.append("")
    lines.append(f"#define {macro_base}_WIDTH  {w}")
    lines.append(f"#define {macro_base}_HEIGHT {h}")
    lines.append(f"#define {macro_base}_TRANSPARENT_KEY 0x{MAGIC_TRANSPARENT:04X}")
    lines.append("")
    lines.append(f"static const uint16_t {sprite_name}_pixels[{w * h}] = {{")

    per_line = 12
    for i in range(0, len(out), per_line):
        chunk = out[i:i + per_line]
        lines.append("    " + ", ".join(f"0x{v:04X}" for v in chunk) + ",")

    lines.append("};")
    lines.append("")

    header_path.write_text("\n".join(lines))
    print(f"Wrote {header_path} ({w}x{h} = {w*h} pixels, {n_transparent} transparent)")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("input_png", type=Path)
    ap.add_argument("output_header", type=Path)
    ap.add_argument("sprite_name", help="C identifier prefix, e.g. player_idle_0")
    ap.add_argument("--alpha-threshold", type=int, default=128,
                     help="Alpha below this becomes transparent (default 128)")
    ap.add_argument("--transparent", type=str, default=None,
                     help="Also treat this literal r,g,b as transparent, e.g. 255,0,255")
    ap.add_argument("--strip-corner-markers", action="store_true",
                     help="Replace dafluffypotato export-tool alignment dots "
                          "(magenta at 1,0 / cyan at w-1,0) with nearby pixel color")
    args = ap.parse_args()

    transparent_key = None
    if args.transparent:
        r, g, b = (int(v) for v in args.transparent.split(","))
        transparent_key = (r, g, b)

    convert(args.input_png, args.output_header, args.sprite_name,
            args.alpha_threshold, transparent_key, args.strip_corner_markers)


if __name__ == "__main__":
    main()