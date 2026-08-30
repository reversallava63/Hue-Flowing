#ifndef SPRITES_ALL_H
#define SPRITES_ALL_H

#include <stdint.h>

// ---- Tiles (uppercase macros) ----
#include "tile_0.h"
#include "tile_1.h"
#include "tile_2.h"
#include "tile_3.h"
#include "tile_4.h"
#include "tile_5.h"
#include "tile_6.h"
#include "tile_7.h"
#include "tile_8.h"
#include "jump_icon.h"

static const uint16_t *tile_pixels[9] = {
    tile_0_pixels, tile_1_pixels, tile_2_pixels,
    tile_3_pixels, tile_4_pixels, tile_5_pixels,
    tile_6_pixels, tile_7_pixels, tile_8_pixels
};
static const uint16_t tile_w[9] = {
    TILE_0_WIDTH, TILE_1_WIDTH, TILE_2_WIDTH,
    TILE_3_WIDTH, TILE_4_WIDTH, TILE_5_WIDTH,
    TILE_6_WIDTH, TILE_7_WIDTH, TILE_8_WIDTH
};
static const uint16_t tile_h[9] = {
    TILE_0_HEIGHT, TILE_1_HEIGHT, TILE_2_HEIGHT,
    TILE_3_HEIGHT, TILE_4_HEIGHT, TILE_5_HEIGHT,
    TILE_6_HEIGHT, TILE_7_HEIGHT, TILE_8_HEIGHT
};
static const int tile_y_offset[9] = { -3, -3, -3, 0, 0, 0, 0, 0, 0 };

// ---- Player animations ----
#include "player_idle_0.h"
#include "player_idle_1.h"
#include "player_idle_2.h"
#include "player_idle_3.h"
static const uint16_t *player_idle_pixels[4] = {
    player_idle_0_pixels, player_idle_1_pixels,
    player_idle_2_pixels, player_idle_3_pixels
};
static const int player_idle_width = PLAYER_IDLE_0_WIDTH;
static const int player_idle_height = PLAYER_IDLE_0_HEIGHT;
static const int player_idle_frame_hold[4] = {60,5,5,5};

#include "player_run_0.h"
#include "player_run_1.h"
#include "player_run_2.h"
#include "player_run_3.h"
#include "player_run_4.h"
#include "player_run_5.h"
#include "player_run_6.h"
#include "player_run_7.h"
static const uint16_t *player_run_pixels[8] = {
    player_run_0_pixels, player_run_1_pixels, player_run_2_pixels, player_run_3_pixels,
    player_run_4_pixels, player_run_5_pixels, player_run_6_pixels, player_run_7_pixels
};
static const int player_run_width = PLAYER_RUN_0_WIDTH;
static const int player_run_height = PLAYER_RUN_0_HEIGHT;

#include "player_jump_0.h"
static const uint16_t *player_jump_pixels[1] = { player_jump_0_pixels };
static const int player_jump_width = PLAYER_JUMP_0_WIDTH;
static const int player_jump_height = PLAYER_JUMP_0_HEIGHT;

#include "player_slide_0.h"
static const uint16_t *player_slide_pixels[1] = { player_slide_0_pixels };
static const int player_slide_width = PLAYER_SLIDE_0_WIDTH;
static const int player_slide_height = PLAYER_SLIDE_0_HEIGHT;

// ---- Decor (uppercase macros) ----
#include "decor_row0_tile0.h"
#include "decor_row1_tile0.h"
#include "decor_row1_tile1.h"
#include "decor_row1_tile2.h"
#include "decor_row2_tile0.h"
#include "decor_row2_tile1.h"
#include "decor_row2_tile2.h"

static const uint16_t *decor_sprites[3][3] = {
    {decor_row0_tile0_pixels, NULL, NULL},
    {decor_row1_tile0_pixels, decor_row1_tile1_pixels, decor_row1_tile2_pixels},
    {decor_row2_tile0_pixels, decor_row2_tile1_pixels, decor_row2_tile2_pixels}
};
static const uint16_t decor_w[3][3] = {
    {DECOR_ROW0_TILE0_WIDTH, 0, 0},
    {DECOR_ROW1_TILE0_WIDTH, DECOR_ROW1_TILE1_WIDTH, DECOR_ROW1_TILE2_WIDTH},
    {DECOR_ROW2_TILE0_WIDTH, DECOR_ROW2_TILE1_WIDTH, DECOR_ROW2_TILE2_WIDTH}
};
static const uint16_t decor_h[3][3] = {
    {DECOR_ROW0_TILE0_HEIGHT, 0, 0},
    {DECOR_ROW1_TILE0_HEIGHT, DECOR_ROW1_TILE1_HEIGHT, DECOR_ROW1_TILE2_HEIGHT},
    {DECOR_ROW2_TILE0_HEIGHT, DECOR_ROW2_TILE1_HEIGHT, DECOR_ROW2_TILE2_HEIGHT}
};

// ---- Foliage (uppercase macros) ----
#include "foliage_row0_tile0.h"
#include "foliage_row0_tile1.h"
#include "foliage_row0_tile2.h"
#include "foliage_row0_tile3.h"
#include "foliage_row1_tile0.h"
#include "foliage_row1_tile1.h"
#include "foliage_row1_tile2.h"
#include "foliage_row1_tile3.h"
#include "foliage_row2_tile0.h"
#include "foliage_row2_tile1.h"
#include "foliage_row2_tile2.h"
#include "foliage_row2_tile3.h"
#include "foliage_row3_tile0.h"
#include "foliage_row3_tile1.h"
#include "foliage_row4_tile0.h"

static const uint16_t *foliage_sprites[5][4] = {
    {foliage_row0_tile0_pixels, foliage_row0_tile1_pixels, foliage_row0_tile2_pixels, foliage_row0_tile3_pixels},
    {foliage_row1_tile0_pixels, foliage_row1_tile1_pixels, foliage_row1_tile2_pixels, foliage_row1_tile3_pixels},
    {foliage_row2_tile0_pixels, foliage_row2_tile1_pixels, foliage_row2_tile2_pixels, foliage_row2_tile3_pixels},
    {foliage_row3_tile0_pixels, foliage_row3_tile1_pixels, NULL, NULL},
    {foliage_row4_tile0_pixels, NULL, NULL, NULL}
};
static const uint16_t foliage_w[5][4] = {
    {FOLIAGE_ROW0_TILE0_WIDTH, FOLIAGE_ROW0_TILE1_WIDTH, FOLIAGE_ROW0_TILE2_WIDTH, FOLIAGE_ROW0_TILE3_WIDTH},
    {FOLIAGE_ROW1_TILE0_WIDTH, FOLIAGE_ROW1_TILE1_WIDTH, FOLIAGE_ROW1_TILE2_WIDTH, FOLIAGE_ROW1_TILE3_WIDTH},
    {FOLIAGE_ROW2_TILE0_WIDTH, FOLIAGE_ROW2_TILE1_WIDTH, FOLIAGE_ROW2_TILE2_WIDTH, FOLIAGE_ROW2_TILE3_WIDTH},
    {FOLIAGE_ROW3_TILE0_WIDTH, FOLIAGE_ROW3_TILE1_WIDTH, 0, 0},
    {FOLIAGE_ROW4_TILE0_WIDTH, 0, 0, 0}
};
static const uint16_t foliage_h[5][4] = {
    {FOLIAGE_ROW0_TILE0_HEIGHT, FOLIAGE_ROW0_TILE1_HEIGHT, FOLIAGE_ROW0_TILE2_HEIGHT, FOLIAGE_ROW0_TILE3_HEIGHT},
    {FOLIAGE_ROW1_TILE0_HEIGHT, FOLIAGE_ROW1_TILE1_HEIGHT, FOLIAGE_ROW1_TILE2_HEIGHT, FOLIAGE_ROW1_TILE3_HEIGHT},
    {FOLIAGE_ROW2_TILE0_HEIGHT, FOLIAGE_ROW2_TILE1_HEIGHT, FOLIAGE_ROW2_TILE2_HEIGHT, FOLIAGE_ROW2_TILE3_HEIGHT},
    {FOLIAGE_ROW3_TILE0_HEIGHT, FOLIAGE_ROW3_TILE1_HEIGHT, 0, 0},
    {FOLIAGE_ROW4_TILE0_HEIGHT, 0, 0, 0}
};

// ---- Grass (uppercase macros) ----
#include "grass_0.h"
#include "grass_1.h"
#include "grass_2.h"
#include "grass_3.h"
#include "grass_4.h"
#include "grass_5.h"
static const uint16_t *grass_pixels[6] = {
    grass_0_pixels, grass_1_pixels, grass_2_pixels,
    grass_3_pixels, grass_4_pixels, grass_5_pixels
};
static const uint16_t grass_w = GRASS_0_WIDTH;
static const uint16_t grass_h = GRASS_0_HEIGHT;

// ---- Entity animations (uppercase macros) ----
#include "jump_upgrade_0.h"
#include "jump_upgrade_1.h"
#include "jump_upgrade_2.h"
#include "jump_upgrade_3.h"
#include "jump_upgrade_4.h"
#include "jump_upgrade_5.h"
#include "jump_upgrade_6.h"
#include "jump_upgrade_7.h"
#include "jump_upgrade_8.h"
#include "jump_upgrade_9.h"
#include "jump_upgrade_10.h"
#include "jump_upgrade_11.h"
#include "jump_upgrade_12.h"
#include "jump_upgrade_13.h"
#include "jump_upgrade_14.h"
#include "jump_upgrade_15.h"
#include "jump_upgrade_16.h"
#define JUMP_UPGRADE_FRAMES 17
static const uint16_t *jump_upgrade_pixels[JUMP_UPGRADE_FRAMES] = {
    jump_upgrade_0_pixels, jump_upgrade_1_pixels, jump_upgrade_2_pixels,
    jump_upgrade_3_pixels, jump_upgrade_4_pixels, jump_upgrade_5_pixels,
    jump_upgrade_6_pixels, jump_upgrade_7_pixels, jump_upgrade_8_pixels,
    jump_upgrade_9_pixels, jump_upgrade_10_pixels, jump_upgrade_11_pixels,
    jump_upgrade_12_pixels, jump_upgrade_13_pixels, jump_upgrade_14_pixels,
    jump_upgrade_15_pixels, jump_upgrade_16_pixels
};
static const int jump_upgrade_width = JUMP_UPGRADE_0_WIDTH;
static const int jump_upgrade_height = JUMP_UPGRADE_0_HEIGHT;

// Jumpanim
#include "jumpanim_0.h"
#include "jumpanim_1.h"
#include "jumpanim_2.h"
#include "jumpanim_3.h"
#include "jumpanim_4.h"
#include "jumpanim_5.h"
#define JUMPANIM_FRAMES 6
static const uint16_t *jumpanim_pixels[JUMPANIM_FRAMES] = {
    jumpanim_0_pixels, jumpanim_1_pixels, jumpanim_2_pixels,
    jumpanim_3_pixels, jumpanim_4_pixels, jumpanim_5_pixels
};
static const int jumpanim_width = JUMPANIM_0_WIDTH;
static const int jumpanim_height = JUMPANIM_0_HEIGHT;

// Jumpanim2
#include "jumpanim2_0.h"
#include "jumpanim2_1.h"
#include "jumpanim2_2.h"
#include "jumpanim2_3.h"
#include "jumpanim2_4.h"
#include "jumpanim2_5.h"
#define JUMPANIM2_FRAMES 6
static const uint16_t *jumpanim2_pixels[JUMPANIM2_FRAMES] = {
    jumpanim2_0_pixels, jumpanim2_1_pixels, jumpanim2_2_pixels,
    jumpanim2_3_pixels, jumpanim2_4_pixels, jumpanim2_5_pixels
};
static const int jumpanim2_width = JUMPANIM2_0_WIDTH;
static const int jumpanim2_height = JUMPANIM2_0_HEIGHT;

// Landanim
#include "landanim_0.h"
#include "landanim_1.h"
#include "landanim_2.h"
#include "landanim_3.h"
#include "landanim_4.h"
#include "landanim_5.h"
#define LANDANIM_FRAMES 6
static const uint16_t *landanim_pixels[LANDANIM_FRAMES] = {
    landanim_0_pixels, landanim_1_pixels, landanim_2_pixels,
    landanim_3_pixels, landanim_4_pixels, landanim_5_pixels
};
static const int landanim_width = LANDANIM_0_WIDTH;
static const int landanim_height = LANDANIM_0_HEIGHT;

// Pillar
#include "pillar_0.h"
#include "pillar_1.h"
#include "pillar_2.h"
#include "pillar_3.h"
#include "pillar_4.h"
#define PILLAR_FRAMES 5
static const uint16_t *pillar_pixels[PILLAR_FRAMES] = {
    pillar_0_pixels, pillar_1_pixels, pillar_2_pixels,
    pillar_3_pixels, pillar_4_pixels
};
static const int pillar_width = PILLAR_0_WIDTH;
static const int pillar_height = PILLAR_0_HEIGHT;

// Turnanim
#include "turnanim_0.h"
#include "turnanim_1.h"
#include "turnanim_2.h"
#include "turnanim_3.h"
#include "turnanim_4.h"
#include "turnanim_5.h"
#define TURNANIM_FRAMES 6
static const uint16_t *turnanim_pixels[TURNANIM_FRAMES] = {
    turnanim_0_pixels, turnanim_1_pixels, turnanim_2_pixels,
    turnanim_3_pixels, turnanim_4_pixels, turnanim_5_pixels
};
static const int turnanim_width = TURNANIM_0_WIDTH;
static const int turnanim_height = TURNANIM_0_HEIGHT;

#endif