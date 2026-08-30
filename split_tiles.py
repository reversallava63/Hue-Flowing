#!/usr/bin/env python3
from PIL import Image
import os

src = "../Hue Flowing Source/data/images/spritesheets/tiles.png"
out_dir = "sprites_h"   # saves PNGs here
os.makedirs(out_dir, exist_ok=True)

img = Image.open(src)
tile_w, tile_h = 16, 16
cols = img.width // tile_w
rows = img.height // tile_h

idx = 0
for row in range(rows):
    for col in range(cols):
        x = col * tile_w
        y = row * tile_h
        tile = img.crop((x, y, x + tile_w, y + tile_h))
        tile.save(os.path.join(out_dir, f"tile_{idx}.png"))
        idx += 1