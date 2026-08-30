#!/usr/bin/env python3
"""
slice_spritesheet.py

Exact port of the game's data/scripts/spritesheet_loader.py::load_spritesheet()
algorithm (which used pygame.get_at()), reimplemented with Pillow so we can run
it standalone. Slices a packed spritesheet into individual tile images using
the same marker-pixel convention:

  - A yellow pixel (255,255,0) at x=0 marks a row that contains tiles.
  - On that row, a magenta pixel (255,0,255) marks the top-left corner of a
    tile (pixel is INSIDE the tile, not a border pixel to strip).
  - Scanning right from that magenta pixel, the next cyan pixel (0,255,255)
    marks where the tile's width ends.
  - Scanning down from the magenta pixel, the next cyan pixel marks where the
    tile's height ends.
  - The actual tile content is cropped from (x+1, row+1) with size
    (width-1, height-1) -- i.e. one pixel inset from the magenta corner,
    which strips the marker pixels themselves out of the final image.

Outputs one PNG per tile, in row-major order, plus prints a manifest so you
know which output file corresponds to which (row, tile_index_in_row).

Usage:
    python3 slice_spritesheet.py tiles.png out_dir/
"""

import sys
from pathlib import Path
from PIL import Image

MAGENTA = (255, 0, 255)
CYAN = (0, 255, 255)
YELLOW = (255, 255, 0)


def load_spritesheet(im: Image.Image):
    im = im.convert("RGB")
    w, h = im.size
    px = im.load()

    rows = []
    for y in range(h):
        if px[0, y] == YELLOW:
            rows.append(y)

    sheet_data = []  # list of rows, each a list of cropped Images
    for row in rows:
        row_content = []
        x = 0
        while x < w:
            if px[x, row] == MAGENTA:
                x2 = 0
                while True:
                    x2 += 1
                    if x + x2 >= w:
                        raise ValueError(f"No closing cyan found for tile at x={x},row={row} (ran off sheet width)")
                    if px[x + x2, row] == CYAN:
                        width = x2
                        break
                y2 = 0
                while True:
                    y2 += 1
                    if row + y2 >= h:
                        raise ValueError(f"No closing cyan found for tile at x={x},row={row} (ran off sheet height)")
                    if px[x, row + y2] == CYAN:
                        height = y2
                        break
                tile = im.crop((x + 1, row + 1, x + width, row + height))
                row_content.append(tile)
                x = x + width  # continue scanning past this tile
            else:
                x += 1
        sheet_data.append(row_content)
    return sheet_data


def main():
    if len(sys.argv) != 3:
        print("Usage: python3 slice_spritesheet.py <input.png> <output_dir>")
        sys.exit(1)

    in_path = Path(sys.argv[1])
    out_dir = Path(sys.argv[2])
    out_dir.mkdir(parents=True, exist_ok=True)

    im = Image.open(in_path)
    sheet = load_spritesheet(im)

    manifest = []
    for row_i, row in enumerate(sheet):
        for tile_i, tile_img in enumerate(row):
            fname = f"row{row_i}_tile{tile_i}.png"
            tile_img.save(out_dir / fname)
            manifest.append((row_i, tile_i, fname, tile_img.size))

    print(f"Sliced {len(manifest)} tiles from {in_path.name} ({len(sheet)} rows)")
    for row_i, tile_i, fname, size in manifest:
        print(f"  row {row_i} tile {tile_i}: {fname}  size={size}")


if __name__ == "__main__":
    main()
