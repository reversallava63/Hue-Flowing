#!/bin/bash
# Run from project root: /.../numworks/hue_eadk

echo "Regenerating level_data.h..."
python3 src/level_to_c.py "../Hue Flowing Source/data/levels/main.json" level_data.h LEVEL_MAIN

echo "Splitting tiles.png..."
python3 split_tiles.py

echo "Converting tiles..."
for i in {0..8}; do
  python3 src/png_to_rgb565.py "sprites_h/tile_$i.png" "sprites_h/tile_$i.h" "tile_$i"
done

echo "Converting player sprites..."
for i in 0 1 2 3; do
  python3 src/png_to_rgb565.py "../Hue Flowing Source/data/images/animations/player_idle/img_$i.png" "sprites_h/player_idle_$i.h" "player_idle_$i"
done
for i in {0..7}; do
  python3 src/png_to_rgb565.py "../Hue Flowing Source/data/images/animations/player_run/img_$i.png" "sprites_h/player_run_$i.h" "player_run_$i"
done
python3 src/png_to_rgb565.py "../Hue Flowing Source/data/images/animations/player_jump/img_0.png" "sprites_h/player_jump_0.h" "player_jump_0"
python3 src/png_to_rgb565.py "../Hue Flowing Source/data/images/animations/player_slide/img_0.png" "sprites_h/player_slide_0.h" "player_slide_0"

echo "Converting turn animation..."
for i in {0..5}; do
  python3 src/png_to_rgb565.py "../Hue Flowing Source/data/images/animations/turnanim_idle/img_$i.png" "sprites_h/turn_$i.h" "turn_$i"
done

echo "All done! Now run: make clean && make"