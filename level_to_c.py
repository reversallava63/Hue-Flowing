#!/usr/bin/env python3
import json, sys

def main():
    if len(sys.argv) != 4:
        print("usage: level_to_c.py <input.json> <output.h> <SYMBOL_PREFIX>")
        sys.exit(1)

    in_path, out_path, prefix = sys.argv[1], sys.argv[2], sys.argv[3]

    with open(in_path, 'r') as f:
        data = json.load(f)

    tile_map = data['map']
    off_grid = data.get('off_grid_map', {})

    solid_coords = []
    tile_index_map = {}
    tile_layer_map = {}
    spawn_x = spawn_y = None
    camera_blocks = []   # (x, y, dir)
    decor_list = []      # (x, y, sheet, tile, layer)
    foliage_list = []    # (x, y, sheet, tile, layer)
    grass_list = []      # (x, y)
    entity_list = []     # (x, y, ent_type, sub, entity_id, layer)

    # Parse grid tiles and inline entities (spawn, jump_upgrade, camera blocks).
    # NOTE: the original game's `load_entities()` scans BOTH the on-grid map
    # and the off-grid map for type[0] == 'entities' (see TileMap.tile_filter,
    # skip_grid_tiles=False). The old version of this script only looked at
    # off_grid_map for entities, so any spawn point / jump_upgrade / pillar
    # placed directly on the tile grid was silently dropped.
    for key, layers in tile_map.items():
        x_str, y_str = key.split(';')
        x, y = int(x_str), int(y_str)
        for layer_key, tile in layers.items():
            layer_id = int(layer_key)
            typ = tile['type']
            px, py = tile['raw'][0]  # pixel coordinates, valid for every on-grid tile type
            if typ[0] == 'tiles':
                tile_index = typ[1]
                solid_coords.append((x, y))
                tile_index_map[(x, y)] = tile_index
                tile_layer_map[(x, y)] = layer_id
            elif typ[0] == 'decor':
                decor_list.append((px, py, typ[1], typ[2], layer_id))
            elif typ[0] == 'foliage':
                foliage_list.append((px, py, typ[1], typ[2], layer_id))
            elif typ[0] == 'grass':
                grass_list.append((px, py))
            elif typ[0] == 'entities':
                sub = typ[1]
                if sub == 0:
                    spawn_x, spawn_y = px, py
                elif sub == 1:
                    eid = tile.get('entity_id', 0)
                    entity_list.append((px, py, sub, typ[2] if len(typ) > 2 else 0, eid, layer_id))
                elif sub in [2, 3, 4, 5]:
                    # camera block: direction 0=right,1=left,2=down,3=up
                    dir = sub - 2
                    camera_blocks.append((px, py, dir))
                elif sub == 9:
                    eid = tile.get('entity_id', 0)
                    entity_list.append((px, py, sub, typ[2] if len(typ) > 2 else 0, eid, layer_id))
                # sub in [6,7,8] (jumpanim/jumpanim2/landanim) are transient
                # VFX cues triggered at runtime in the original, not persistent
                # level objects -- intentionally skipped here.

    # Parse off-grid entities for spawn and other decorations/entities
    for layer, entities in off_grid.items():
        layer_id = int(layer)
        for ent in entities:
            typ = ent['type']
            pos = ent['pos']  # pixel coordinates
            if typ == ['entities', 0, 0]:
                spawn_x, spawn_y = pos
            elif typ[0] == 'decor':
                decor_list.append((pos[0], pos[1], typ[1], typ[2], layer_id))
            elif typ[0] == 'foliage':
                foliage_list.append((pos[0], pos[1], typ[1], typ[2], layer_id))
            elif typ[0] == 'grass':
                grass_list.append((pos[0], pos[1]))
            elif typ[0] == 'entities' and typ[1] not in [2, 3, 4, 5]:
                # store entity (excluding camera blocks)
                entity_list.append((pos[0], pos[1], typ[1], typ[2], ent.get('entity_id', 0), layer_id))

    if not solid_coords:
        print("No solid tiles found.")
        sys.exit(1)

    min_x = min(c[0] for c in solid_coords)
    max_x = max(c[0] for c in solid_coords)
    min_y = min(c[1] for c in solid_coords)
    max_y = max(c[1] for c in solid_coords)

    width = max_x - min_x + 1
    height = max_y - min_y + 1

    grid = [[0 for _ in range(width)] for _ in range(height)]
    tile_idx_grid = [[-1 for _ in range(width)] for _ in range(height)]
    tile_layer_grid = [[0 for _ in range(width)] for _ in range(height)]

    for (x, y) in solid_coords:
        gx = x - min_x
        gy = y - min_y
        grid[gy][gx] = 1
        tile_idx_grid[gy][gx] = tile_index_map[(x, y)]
        tile_layer_grid[gy][gx] = tile_layer_map[(x, y)]

    with open(out_path, 'w') as f:
        f.write(f"// Auto-generated from {in_path}\n")
        f.write(f"#ifndef {prefix}_H\n#define {prefix}_H\n\n")
        f.write(f"#define {prefix}_WIDTH  {width}\n")
        f.write(f"#define {prefix}_HEIGHT {height}\n")
        f.write(f"#define {prefix}_ORIGIN_X {min_x}\n")
        f.write(f"#define {prefix}_ORIGIN_Y {min_y}\n")
        if spawn_x is not None and spawn_y is not None:
            f.write(f"#define {prefix}_SPAWN_X {spawn_x}\n")
            f.write(f"#define {prefix}_SPAWN_Y {spawn_y}\n")
        # Camera blocks
        f.write(f"#define {prefix}_CAMERA_BLOCK_COUNT {len(camera_blocks)}\n")
        f.write(f"static const int {prefix}_CAMERA_BLOCKS[{prefix}_CAMERA_BLOCK_COUNT][3] = {{\n")
        for (bx, by, d) in camera_blocks:
            f.write(f"  {{{bx},{by},{d}}},\n")
        f.write("};\n\n")

        # Decor: x, y, sheet, tile, layer
        f.write(f"#define {prefix}_DECOR_COUNT {len(decor_list)}\n")
        f.write(f"static const int {prefix}_DECOR[{prefix}_DECOR_COUNT][5] = {{\n")
        for (x, y, sheet, tile, layer_id) in decor_list:
            f.write(f"  {{{x},{y},{sheet},{tile},{layer_id}}},\n")
        f.write("};\n\n")

        # Foliage: x, y, sheet, tile, layer
        f.write(f"#define {prefix}_FOLIAGE_COUNT {len(foliage_list)}\n")
        f.write(f"static const int {prefix}_FOLIAGE[{prefix}_FOLIAGE_COUNT][5] = {{\n")
        for (x, y, sheet, tile, layer_id) in foliage_list:
            f.write(f"  {{{x},{y},{sheet},{tile},{layer_id}}},\n")
        f.write("};\n\n")

        # Grass
        f.write(f"#define {prefix}_GRASS_COUNT {len(grass_list)}\n")
        f.write(f"static const int {prefix}_GRASS[{prefix}_GRASS_COUNT][2] = {{\n")
        for (x, y) in grass_list:
            f.write(f"  {{{x},{y}}},\n")
        f.write("};\n\n")

        # Entities (non-camera): x, y, type, sub, entity_id, layer
        f.write(f"#define {prefix}_ENTITY_COUNT {len(entity_list)}\n")
        f.write(f"static const int {prefix}_ENTITIES[{prefix}_ENTITY_COUNT][6] = {{\n")
        for (x, y, typ, sub, eid, layer_id) in entity_list:
            f.write(f"  {{{x},{y},{typ},{sub},{eid},{layer_id}}},\n")
        f.write("};\n\n")

        # Tiles
        f.write(f"static const unsigned char {prefix}_GRID[{prefix}_HEIGHT][{prefix}_WIDTH] = {{\n")
        for row in grid:
            f.write("  {" + ",".join(str(v) for v in row) + "},\n")
        f.write("};\n\n")
        f.write(f"static const signed char {prefix}_TILE_IDX[{prefix}_HEIGHT][{prefix}_WIDTH] = {{\n")
        for row in tile_idx_grid:
            f.write("  {" + ",".join(str(v) for v in row) + "},\n")
        f.write("};\n\n")
        f.write(f"static const signed char {prefix}_TILE_LAYER[{prefix}_HEIGHT][{prefix}_WIDTH] = {{\n")
        for row in tile_layer_grid:
            f.write("  {" + ",".join(str(v) for v in row) + "},\n")
        f.write("};\n\n#endif\n")

    print(f"Wrote {out_path}: {width}x{height}, {len(solid_coords)} solid tiles.")
    if spawn_x is not None:
        print(f"Player spawn at ({spawn_x}, {spawn_y})")
    else:
        print("WARNING: no spawn entity (type 0) found on grid or off-grid!")
    print(f"Camera blocks: {len(camera_blocks)}")
    print(f"Decor: {len(decor_list)}")
    print(f"Foliage: {len(foliage_list)}")
    print(f"Grass: {len(grass_list)}")
    print(f"Entities: {len(entity_list)} (includes jump_upgrade + pillar)")

if __name__ == '__main__':
    main()