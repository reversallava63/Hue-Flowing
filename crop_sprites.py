#!/usr/bin/env python3
import os
import sys
from PIL import Image

def crop_image(path):
    img = Image.open(path).convert("RGBA")
    bbox = img.getbbox()  # (left, top, right, bottom) of non‑zero alpha
    if bbox:
        cropped = img.crop(bbox)
        cropped.save(path)
        print(f"Cropped {path}: {img.size} -> {cropped.size}")
    else:
        print(f"Fully transparent, skipping {path}")

def crop_directory(root):
    for dirpath, _, filenames in os.walk(root):
        for fname in filenames:
            if fname.lower().endswith(".png"):
                crop_image(os.path.join(dirpath, fname))

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python crop_sprites.py <directory>")
        sys.exit(1)
    crop_directory(sys.argv[1])