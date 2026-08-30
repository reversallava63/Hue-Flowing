#include <eadk.h>
#include <string.h>
#include <math.h>
#define LEVEL_MAIN
#include "level_data.h"
#include "sprites_h/sprites_all.h"
#include "noise_data.h"

const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Hue Flowing";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;

// ---------- constants ----------
#define TILE_SIZE          16
#define GRAVITY_ACCEL       0.15f
#define GRAVITY_MAX         3.0f
#define MOVE_ACCEL          0.35f
#define MOVE_MAX            2.0f
#define JUMP_VELOCITY      -3.55f
#define INITIAL_MAX_JUMPS   2
#define WALL_JUMP_VY       -3.0f
#define WALL_JUMP_VX_KICK  -2.5f

// Original collision box (not sprite size)
#define PLAYER_W            11
#define PLAYER_H            16

#define TRAIL_MAX           60
#define TRAIL_SIZE          4
#define MAX_BLOBS           32

// --- mask constants ---
#define MASK_SCALE          4
#define CHUNK_SIZE          256
#define CHUNK_BITS          (CHUNK_SIZE / MASK_SCALE)
#define CHUNK_TEXELS        (CHUNK_BITS * CHUNK_BITS)
#define TEXEL_BITS          2
#define CHUNK_BYTES         (CHUNK_TEXELS * TEXEL_BITS / 8)

#define LEVEL_PIXEL_WIDTH   (LEVEL_MAIN_WIDTH * TILE_SIZE)
#define LEVEL_PIXEL_HEIGHT  (LEVEL_MAIN_HEIGHT * TILE_SIZE)
#define CHUNK_COLS          ((LEVEL_PIXEL_WIDTH + CHUNK_SIZE - 1) / CHUNK_SIZE)
#define CHUNK_ROWS          ((LEVEL_PIXEL_HEIGHT + CHUNK_SIZE - 1) / CHUNK_SIZE)

#define LEVEL_ORIGIN_X_PIXELS  (LEVEL_MAIN_ORIGIN_X * TILE_SIZE)
#define LEVEL_ORIGIN_Y_PIXELS  (LEVEL_MAIN_ORIGIN_Y * TILE_SIZE)

#define BASE_PAINT_RADIUS   18
#define UPGRADE_REVEAL_RADIUS 12
#define PILLAR_REVEAL_RADIUS  20

#define SCREEN_MASK_BYTES_PER_ROW  ((EADK_SCREEN_WIDTH + 3) / 4)

// ---------- types ----------
typedef struct { float x, y, w, h; } fRect;
typedef struct { int x, y; eadk_color_t color; } TrailDab;

typedef struct {
    float x, y;
    float vx, vy;
    float phase;
    float decay_rate;
    float swerve;
    unsigned char active;
} PaintBlob;

typedef struct {
    int16_t x, y, w, h;
    const uint16_t *pixels;
    uint8_t flip;
} VisibleEntity;

#define MAX_VISIBLE_ENTITIES 8

// ---------- forward declarations ----------
static float normalize_f(float val, float amt, float target);
static void find_sprite_bounds(const uint16_t *pixels, int w, int h,
                               int *min_x, int *min_y, int *max_x, int *max_y);

// ---------- custom random generator ----------
static uint32_t rng_state = 123456789;
static uint32_t rng_next(void) {
    rng_state = rng_state * 1103515245 + 12345;
    return rng_state;
}
static float frand(void) {
    return (float)rng_next() / (float)0xFFFFFFFF;
}

// ---------- globals ----------
static TrailDab trail[TRAIL_MAX];
static int trail_count = 0, trail_head = 0;
static const eadk_color_t palette[] = { 0xF800, 0xFD20, 0x07E0, 0x001F, 0x781F, 0x07FF };
#define N_COLORS (sizeof(palette)/sizeof(palette[0]))

static PaintBlob blobs[MAX_BLOBS];
static float g_time_global = 0.0f;

static uint8_t world_mask[CHUNK_ROWS][CHUNK_COLS][CHUNK_BYTES];
static uint8_t paint_bits[EADK_SCREEN_HEIGHT][SCREEN_MASK_BYTES_PER_ROW];

// ---------- colour helpers ----------
static inline uint16_t rgb_to_565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

static inline uint16_t lerp_rgb565(uint16_t c1, uint16_t c2, float t) {
    int r1 = (c1 >> 8) & 0xF8;
    int g1 = (c1 >> 3) & 0xFC;
    int b1 = (c1 << 3) & 0xF8;
    int r2 = (c2 >> 8) & 0xF8;
    int g2 = (c2 >> 3) & 0xFC;
    int b2 = (c2 << 3) & 0xF8;
    int r = r1 + (int)((r2 - r1) * t);
    int g = g1 + (int)((g2 - g1) * t);
    int b = b1 + (int)((b2 - b1) * t);
    return rgb_to_565(r, g, b);
}

static uint16_t g_sky_light, g_sky_dark, g_cloud_color;
static uint16_t g_sky_row[EADK_SCREEN_HEIGHT];

static void generate_background() {
    g_sky_light   = rgb_to_565(24, 164, 231);
    g_sky_dark    = rgb_to_565(30, 105, 162);
    g_cloud_color = rgb_to_565(237, 235, 217);
    for (int y = 0; y < EADK_SCREEN_HEIGHT; y++) {
        float t = (float)y / EADK_SCREEN_HEIGHT;
        t = t * t * 0.8f + t * 0.2f;
        g_sky_row[y] = lerp_rgb565(g_sky_light, g_sky_dark, t);
    }
}

// ---------- noise ----------
static inline float sample_noise_bilinear(float u, float v) {
    if (u < 0.0f) u = 0.0f; else if (u > 1.0f) u = 1.0f;
    if (v < 0.0f) v = 0.0f; else if (v > 1.0f) v = 1.0f;
    float fu = u * (NOISE_WIDTH - 1);
    float fv = v * (NOISE_HEIGHT - 1);
    int iu = (int)fu;
    int iv = (int)fv;
    float frac_u = fu - iu;
    float frac_v = fv - iv;
    if (iu < 0) iu = 0; if (iu >= NOISE_WIDTH) iu = NOISE_WIDTH - 1;
    if (iv < 0) iv = 0; if (iv >= NOISE_HEIGHT) iv = NOISE_HEIGHT - 1;
    int iu_next = (iu + 1 < NOISE_WIDTH) ? iu + 1 : iu;
    int iv_next = (iv + 1 < NOISE_HEIGHT) ? iv + 1 : iv;
    float v00 = noise_data[iv * NOISE_WIDTH + iu] * (1.0f/255.0f);
    float v10 = noise_data[iv * NOISE_WIDTH + iu_next] * (1.0f/255.0f);
    float v01 = noise_data[iv_next * NOISE_WIDTH + iu] * (1.0f/255.0f);
    float v11 = noise_data[iv_next * NOISE_WIDTH + iu_next] * (1.0f/255.0f);
    float v0 = v00 + (v10 - v00) * frac_u;
    float v1 = v01 + (v11 - v01) * frac_u;
    return v0 + (v1 - v0) * frac_v;
}

// ---------- background cache (quarter resolution) ----------
#define BG_CACHE_W  (EADK_SCREEN_WIDTH / 4)
#define BG_CACHE_H  (EADK_SCREEN_HEIGHT / 4)
static uint16_t bg_cache[BG_CACHE_H][BG_CACHE_W];

static void generate_background_cache(int cam_x, int cam_y,
                                      float adj_time,
                                      float cos_t_0_07,
                                      float sin_t_0_05,
                                      float cos_t_0_05) {
    const float noise_scale = 0.005f;
    float cam_x_f = cam_x;
    float cam_y_f = cam_y;
    for (int y = 0; y < BG_CACHE_H; y++) {
        uint16_t sky = g_sky_row[y * 4];
        for (int x = 0; x < BG_CACHE_W; x++) {
            float sx = (x * 4) + cam_x_f;
            float sy = (y * 4) + cam_y_f;
            float spx = (x * 4) + cam_x_f * 1.25f;
            float spy = (y * 4) + cam_y_f * 1.25f;

            float u1 = spx * 0.5f * noise_scale - adj_time * 0.02f;
            float v1 = spy * noise_scale + cos_t_0_07 * 0.1f;
            float u2 = sx * noise_scale + sin_t_0_05 * 0.1f;
            float v2 = sy * noise_scale * 3.0f + cos_t_0_05 * 0.1f;
            float u3 = spx * 0.3f * noise_scale - adj_time * 0.035f;
            float v3 = spy * 0.8f * noise_scale + cosf(adj_time * 0.09f + 0.3f) * 0.1f;

            u1 = fmodf(u1, 1.0f); if (u1 < 0.0f) u1 += 1.0f;
            v1 = fmodf(v1, 1.0f); if (v1 < 0.0f) v1 += 1.0f;
            u2 = fmodf(u2, 1.0f); if (u2 < 0.0f) u2 += 1.0f;
            v2 = fmodf(v2, 1.0f); if (v2 < 0.0f) v2 += 1.0f;
            u3 = fmodf(u3, 1.0f); if (u3 < 0.0f) u3 += 1.0f;
            v3 = fmodf(v3, 1.0f); if (v3 < 0.0f) v3 += 1.0f;

            float noise_val = sample_noise_bilinear(u3, v3) * 0.7f +
                              sample_noise_bilinear(u1, v1) * 0.2f +
                              sample_noise_bilinear(u2, v2) * 0.1f;

            float noise_level = (noise_val - 0.5f) * 8.0f;
            if (noise_level < 0.0f) noise_level = 0.0f;
            if (noise_level > 1.0f) noise_level = 1.0f;

            uint16_t base_color = lerp_rgb565(sky, g_cloud_color, noise_level * 0.3f);
            bg_cache[y][x] = base_color;
        }
    }
}

// ---------- screen mask helpers ----------
static inline void clear_screen_mask(void) {
    memset(paint_bits, 0, sizeof(paint_bits));
}

static inline uint8_t get_screen_intensity(int x, int y) {
    if (x < 0 || x >= EADK_SCREEN_WIDTH || y < 0 || y >= EADK_SCREEN_HEIGHT) return 0;
    int byte_idx = x >> 2;
    int shift = (x & 3) * 2;
    return (paint_bits[y][byte_idx] >> shift) & 0x03;
}

static inline void set_screen_intensity(int x, int y, uint8_t val) {
    if (x < 0 || x >= EADK_SCREEN_WIDTH || y < 0 || y >= EADK_SCREEN_HEIGHT) return;
    if (val > 3) val = 3;
    int byte_idx = x >> 2;
    int shift = (x & 3) * 2;
    uint8_t mask = 0x03 << shift;
    paint_bits[y][byte_idx] = (paint_bits[y][byte_idx] & ~mask) | (val << shift);
}

// ---------- world mask helpers ----------
static inline int world_to_chunk_x(int world_x) {
    int local_x = world_x - LEVEL_ORIGIN_X_PIXELS;
    if (local_x < 0) local_x = 0;
    return local_x / CHUNK_SIZE;
}

static inline int world_to_chunk_y(int world_y) {
    int local_y = world_y - LEVEL_ORIGIN_Y_PIXELS;
    if (local_y < 0) local_y = 0;
    return local_y / CHUNK_SIZE;
}

static void clear_world_mask(void) {
    memset(world_mask, 0, sizeof(world_mask));
}

static inline uint8_t get_world_texel(int cx, int cy, int tx, int ty) {
    if (cx < 0 || cx >= CHUNK_COLS || cy < 0 || cy >= CHUNK_ROWS) return 0;
    if (tx < 0 || tx >= CHUNK_BITS || ty < 0 || ty >= CHUNK_BITS) return 0;
    int bit_index = (ty * CHUNK_BITS + tx) * TEXEL_BITS;
    int byte_idx = bit_index >> 3;
    int shift = bit_index & 7;
    return (world_mask[cy][cx][byte_idx] >> shift) & 0x03;
}

static inline void set_world_texel(int cx, int cy, int tx, int ty, uint8_t val) {
    if (cx < 0 || cx >= CHUNK_COLS || cy < 0 || cy >= CHUNK_ROWS) return;
    if (tx < 0 || tx >= CHUNK_BITS || ty < 0 || ty >= CHUNK_BITS) return;
    if (val > 3) val = 3;
    int bit_index = (ty * CHUNK_BITS + tx) * TEXEL_BITS;
    int byte_idx = bit_index >> 3;
    int shift = bit_index & 7;
    uint8_t mask = 0x03 << shift;
    world_mask[cy][cx][byte_idx] = (world_mask[cy][cx][byte_idx] & ~mask) | (val << shift);
}

static void mark_paint(int world_x, int world_y, int radius) {
    if (radius <= 0) return;

    int min_cx = world_to_chunk_x(world_x - radius);
    int max_cx = world_to_chunk_x(world_x + radius);
    int min_cy = world_to_chunk_y(world_y - radius);
    int max_cy = world_to_chunk_y(world_y + radius);

    float r_f = (float)radius;
    float inner_r = r_f * 0.4f;
    float mid_r = r_f * 0.7f;

    for (int cy = min_cy; cy <= max_cy; cy++) {
        if (cy < 0 || cy >= CHUNK_ROWS) continue;
        for (int cx = min_cx; cx <= max_cx; cx++) {
            if (cx < 0 || cx >= CHUNK_COLS) continue;

            int chunk_wx = LEVEL_ORIGIN_X_PIXELS + cx * CHUNK_SIZE;
            int chunk_wy = LEVEL_ORIGIN_Y_PIXELS + cy * CHUNK_SIZE;

            int tx_min = (world_x - radius - chunk_wx) / MASK_SCALE;
            int tx_max = (world_x + radius - chunk_wx) / MASK_SCALE;
            int ty_min = (world_y - radius - chunk_wy) / MASK_SCALE;
            int ty_max = (world_y + radius - chunk_wy) / MASK_SCALE;

            if (tx_min < 0) tx_min = 0;
            if (ty_min < 0) ty_min = 0;
            if (tx_max >= CHUNK_BITS) tx_max = CHUNK_BITS - 1;
            if (ty_max >= CHUNK_BITS) ty_max = CHUNK_BITS - 1;

            for (int ty = ty_min; ty <= ty_max; ty++) {
                int wy = chunk_wy + ty * MASK_SCALE + MASK_SCALE / 2;
                for (int tx = tx_min; tx <= tx_max; tx++) {
                    int wx = chunk_wx + tx * MASK_SCALE + MASK_SCALE / 2;
                    float dx = (float)(wx - world_x);
                    float dy = (float)(wy - world_y);
                    float dist = sqrtf(dx*dx + dy*dy);
                    uint8_t intensity = 0;
                    if (dist <= inner_r) intensity = 3;
                    else if (dist <= mid_r) intensity = 2;
                    else if (dist <= r_f) intensity = 1;
                    if (intensity > 0) {
                        uint8_t current = get_world_texel(cx, cy, tx, ty);
                        if (intensity > current) {
                            set_world_texel(cx, cy, tx, ty, intensity);
                        }
                    }
                }
            }
        }
    }
}

static void build_screen_mask(int cam_x, int cam_y) {
    clear_screen_mask();

    int start_cx = world_to_chunk_x(cam_x);
    int end_cx = world_to_chunk_x(cam_x + EADK_SCREEN_WIDTH);
    int start_cy = world_to_chunk_y(cam_y);
    int end_cy = world_to_chunk_y(cam_y + EADK_SCREEN_HEIGHT);

    for (int cy = start_cy; cy <= end_cy; cy++) {
        if (cy < 0 || cy >= CHUNK_ROWS) continue;
        for (int cx = start_cx; cx <= end_cx; cx++) {
            if (cx < 0 || cx >= CHUNK_COLS) continue;

            for (int ty = 0; ty < CHUNK_BITS; ty++) {
                for (int tx = 0; tx < CHUNK_BITS; tx++) {
                    uint8_t intensity = get_world_texel(cx, cy, tx, ty);
                    if (intensity == 0) continue;

                    int world_x = LEVEL_ORIGIN_X_PIXELS + cx * CHUNK_SIZE + tx * MASK_SCALE;
                    int world_y = LEVEL_ORIGIN_Y_PIXELS + cy * CHUNK_SIZE + ty * MASK_SCALE;
                    int sx = world_x - cam_x;
                    int sy = world_y - cam_y;

                    for (int dy = 0; dy < MASK_SCALE; dy++) {
                        for (int dx = 0; dx < MASK_SCALE; dx++) {
                            int px = sx + dx;
                            int py = sy + dy;
                            if (px >= 0 && px < EADK_SCREEN_WIDTH && py >= 0 && py < EADK_SCREEN_HEIGHT) {
                                set_screen_intensity(px, py, intensity);
                            }
                        }
                    }
                }
            }
        }
    }
}

// ---------- blob management ----------
static void spawn_blob(float x, float y, float angle, float force,
                       float phase, float decay_rate, float swerve_rate) {
    for (int i = 0; i < MAX_BLOBS; i++) {
        if (!blobs[i].active) {
            blobs[i].active = 1;
            blobs[i].x = x;
            blobs[i].y = y;
            blobs[i].vx = cosf(angle) * force;
            blobs[i].vy = sinf(angle) * force;
            blobs[i].phase = phase;
            blobs[i].decay_rate = decay_rate;
            blobs[i].swerve = frand() * swerve_rate;
            return;
        }
    }
}

static void update_blobs(float dt) {
    for (int i = 0; i < MAX_BLOBS; i++) {
        PaintBlob *b = &blobs[i];
        if (!b->active) continue;
        b->vx = normalize_f(b->vx, 0.02f * dt * 60, 0);
        float speed = sqrtf(b->vx * b->vx + b->vy * b->vy);
        if (speed > 0.001f) {
            b->vx += sinf(g_time_global * 100 / speed + b->swerve * 1000) * b->swerve * 0.3f * dt * 60;
            b->vy += sinf(g_time_global * 120 / speed + b->swerve * 1200) * b->swerve * 0.3f * dt * 60;
        }
        b->vy = fminf(2.5f, b->vy + 0.25f * dt * 60);
        b->phase += b->decay_rate * dt * 60;
        if (b->phase >= 8.0f) {
            b->active = 0;
            continue;
        }
        float steps = fmaxf(1.0f, sqrtf(b->vx*b->vx + b->vy*b->vy) * 0.5f);
        int radius = (int)(5.0f * (1.0f - b->phase / 8.0f)) + 2;
        if (radius < 2) radius = 2;
        for (int s = 0; s < (int)steps; s++) {
            b->x += b->vx / steps;
            b->y += b->vy / steps;
            mark_paint((int)b->x, (int)b->y, radius);
        }
    }
}

// ---------- physics ----------
static float normalize_f(float val, float amt, float target) {
    if (val > target + amt) val -= amt;
    else if (val < target - amt) val += amt;
    else val = target;
    return val;
}

static int rects_overlap_f(fRect a, fRect b) {
    return a.x < b.x + b.w && a.x + a.w > b.x &&
           a.y < b.y + b.h && a.y + a.h > b.y;
}

static int tile_solid(int tx, int ty) {
    int gx = tx - LEVEL_MAIN_ORIGIN_X;
    int gy = ty - LEVEL_MAIN_ORIGIN_Y;
    if (gx < 0 || gy < 0 || gx >= LEVEL_MAIN_WIDTH || gy >= LEVEL_MAIN_HEIGHT) return 0;
    return LEVEL_MAIN_GRID[gy][gx];
}

static int tile_index(int tx, int ty) {
    int gx = tx - LEVEL_MAIN_ORIGIN_X;
    int gy = ty - LEVEL_MAIN_ORIGIN_Y;
    if (gx < 0 || gy < 0 || gx >= LEVEL_MAIN_WIDTH || gy >= LEVEL_MAIN_HEIGHT) return -1;
    return LEVEL_MAIN_TILE_IDX[gy][gx];
}

static int tile_layer(int tx, int ty) {
    int gx = tx - LEVEL_MAIN_ORIGIN_X;
    int gy = ty - LEVEL_MAIN_ORIGIN_Y;
    if (gx < 0 || gy < 0 || gx >= LEVEL_MAIN_WIDTH || gy >= LEVEL_MAIN_HEIGHT) return 0;
    return LEVEL_MAIN_TILE_LAYER[gy][gx];
}

// ---------- camera ----------
#define CAM_RIGHT 0
#define CAM_LEFT  1
#define CAM_DOWN  2
#define CAM_UP    3
static float scroll_x, scroll_y;

static int rect_overlap_i(float rx, float ry, float rw, float rh, float bx, float by, float bw, float bh) {
    return rx < bx + bw && rx + rw > bx && ry < by + bh && ry + rh > by;
}

static void update_camera(float center_x, float center_y) {
    scroll_x += (center_x - EADK_SCREEN_WIDTH / 2.0f - scroll_x) / 20.0f;
    {
        float view_x = scroll_x, view_y = scroll_y;
        const float view_w = EADK_SCREEN_WIDTH, view_h = EADK_SCREEN_HEIGHT;
        for (int pass = 0; pass < 2; pass++) {
            int want_dir = (pass == 0) ? CAM_LEFT : CAM_RIGHT;
            for (int i = 0; i < LEVEL_MAIN_CAMERA_BLOCK_COUNT; i++) {
                float bx = (float)LEVEL_MAIN_CAMERA_BLOCKS[i][0];
                float by = (float)LEVEL_MAIN_CAMERA_BLOCKS[i][1];
                int dir = LEVEL_MAIN_CAMERA_BLOCKS[i][2];
                if (dir != want_dir) continue;
                if (!rect_overlap_i(view_x, view_y, view_w, view_h, bx, by, 16, 16)) continue;
                if (dir == CAM_RIGHT) view_x = bx + 16;
                else view_x = bx - view_w;
                scroll_x = view_x;
                scroll_y = view_y;
            }
        }
    }
    scroll_y += (center_y - EADK_SCREEN_HEIGHT / 2.0f - scroll_y) / 20.0f;
    {
        float view_x = scroll_x, view_y = scroll_y;
        const float view_w = EADK_SCREEN_WIDTH, view_h = EADK_SCREEN_HEIGHT;
        for (int pass = 0; pass < 2; pass++) {
            int want_dir = (pass == 0) ? CAM_UP : CAM_DOWN;
            for (int i = 0; i < LEVEL_MAIN_CAMERA_BLOCK_COUNT; i++) {
                float bx = (float)LEVEL_MAIN_CAMERA_BLOCKS[i][0];
                float by = (float)LEVEL_MAIN_CAMERA_BLOCKS[i][1];
                int dir = LEVEL_MAIN_CAMERA_BLOCKS[i][2];
                if (dir != want_dir) continue;
                if (!rect_overlap_i(view_x, view_y, view_w, view_h, bx, by, 16, 16)) continue;
                if (dir == CAM_UP) view_y = by - view_h;
                else view_y = by + 16;
                scroll_x = view_x;
                scroll_y = view_y;
            }
        }
    }
}

static void clamp_player_to_view(float *px, float *py) {
    int rsx = (int)scroll_x, rsy = (int)scroll_y;
    if (*px + PLAYER_W > rsx + EADK_SCREEN_WIDTH) *px = (float)(rsx + EADK_SCREEN_WIDTH - PLAYER_W);
    if (*px < rsx) *px = (float)rsx;
    if (*py < rsy) *py = (float)rsy;
}

#define MAX_NEARBY 25
static int get_nearby_rects(float px, float py, fRect *out) {
    int tx = (int)(px / TILE_SIZE), ty = (int)(py / TILE_SIZE);
    int n = 0;
    for (int dy = -2; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            if (tile_solid(tx + dx, ty + dy)) {
                out[n].x = (float)((tx + dx) * TILE_SIZE);
                out[n].y = (float)((ty + dy) * TILE_SIZE);
                out[n].w = TILE_SIZE; out[n].h = TILE_SIZE;
                n++;
                if (n >= MAX_NEARBY) return n;
            }
        }
    }
    return n;
}

typedef struct { int top, left, right, bottom; } Collisions;

static Collisions entity_move(float *px, float *py, float mx, float my) {
    Collisions c = {0,0,0,0};
    fRect nearby[MAX_NEARBY];
    *px += mx;
    fRect pr = {*px, *py, PLAYER_W, PLAYER_H};
    int n = get_nearby_rects(*px, *py, nearby);
    for (int i=0; i<n; i++) {
        if (rects_overlap_f(pr, nearby[i])) {
            if (mx > 0) { *px = nearby[i].x - PLAYER_W; c.right = 1; }
            if (mx < 0) { *px = nearby[i].x + nearby[i].w; c.left = 1; }
            pr.x = *px;
        }
    }
    *py += my;
    pr = (fRect){*px, *py, PLAYER_W, PLAYER_H};
    n = get_nearby_rects(*px, *py, nearby);
    for (int i=0; i<n; i++) {
        if (rects_overlap_f(pr, nearby[i])) {
            if (my > 0) { *py = nearby[i].y - PLAYER_H; c.bottom = 1; }
            if (my < 0) { *py = nearby[i].y + nearby[i].h; c.top = 1; }
            pr.y = *py;
        }
    }
    return c;
}

// ---------- rendering ----------
static uint16_t line_buffer[EADK_SCREEN_WIDTH];

static void draw_sprite_row(int screen_x, int screen_y,
                            const uint16_t *pixels, int w, int h,
                            int flip_h, int row_offset, int skip_transparent) {
    int y = screen_y + row_offset;
    if (y < 0 || y >= EADK_SCREEN_HEIGHT) return;
    int src_y = row_offset;
    if (src_y < 0 || src_y >= h) return;

    const uint16_t *src_line = &pixels[src_y * w];
    int draw_x = screen_x;
    int draw_w = w;

    if (draw_x < 0) {
        src_line -= draw_x;
        draw_w += draw_x;
        draw_x = 0;
    }
    if (draw_x + draw_w > EADK_SCREEN_WIDTH) {
        draw_w = EADK_SCREEN_WIDTH - draw_x;
    }
    if (draw_w <= 0) return;

    if (!flip_h) {
        if (!skip_transparent) {
            memcpy(&line_buffer[draw_x], src_line, draw_w * sizeof(uint16_t));
        } else {
            for (int x = 0; x < draw_w; x++) {
                uint16_t pixel = src_line[x];
                if (pixel != 0x0001 && pixel != 0x0000) line_buffer[draw_x + x] = pixel;
            }
        }
    } else {
        for (int x = 0; x < draw_w; x++) {
            uint16_t pixel = src_line[draw_w - 1 - x];
            if (!skip_transparent || (pixel != 0x0001 && pixel != 0x0000)) {
                line_buffer[draw_x + x] = pixel;
            }
        }
    }
}

static void draw_player_row_squashed(int screen_x, int feet_screen_y,
                                     const uint16_t *pixels, int w, int h,
                                     int flip_h, float scale_y, int y) {
    int scaled_h = (int)(h * scale_y + 0.5f);
    if (scaled_h < 1) scaled_h = 1;
    int top_y = feet_screen_y - scaled_h;
    if (y < top_y || y >= feet_screen_y) return;
    int src_y = (int)((y - top_y) / scale_y);
    if (src_y < 0) src_y = 0;
    if (src_y >= h) src_y = h - 1;
    draw_sprite_row(screen_x, top_y, pixels, w, h, flip_h, src_y, 1);
}

static void draw_player_row_rotated(int center_x, int center_y,
                                    const uint16_t *pixels, int pw, int ph,
                                    int flip_h, float cos_r, float sin_r,
                                    int bbox_half, int y) {
    if (y < center_y - bbox_half || y >= center_y + bbox_half) return;
    int ly = y - center_y;
    for (int dx = -bbox_half; dx < bbox_half; dx++) {
        int screen_x = center_x + dx;
        if (screen_x < 0 || screen_x >= EADK_SCREEN_WIDTH) continue;
        float sxf = dx * cos_r + ly * sin_r;
        float syf = -dx * sin_r + ly * cos_r;
        int sx = (int)(sxf + pw / 2.0f);
        int sy = (int)(syf + ph / 2.0f);
        if (flip_h) sx = pw - 1 - sx;
        if (sx < 0 || sx >= pw || sy < 0 || sy >= ph) continue;
        uint16_t c = pixels[sy * pw + sx];
        if (c != 0x0001 && c != 0x0000) line_buffer[screen_x] = c;
    }
}

static void draw_sprite_row_sway(int screen_x, int screen_y,
                                 const uint16_t *pixels, int w, int h,
                                 int flip_h, float seed, float m_clock,
                                 int row_offset) {
    int y = screen_y + row_offset;
    if (y < 0 || y >= EADK_SCREEN_HEIGHT) return;
    int src_y = row_offset;
    if (src_y < 0 || src_y >= h) return;

    float row_frac = (h > 1) ? (1.0f - (float)row_offset / (float)(h - 1)) : 0.0f;
    float sway = sinf(m_clock * 2.5f + 2.7f * seed) * 4.0f * (0.5f + 0.5f * row_frac);
    int shift_x = (int)sway;
    float bob = sinf(m_clock * 1.8f + 1.3f * seed) * 2.0f;
    int shift_y = (int)bob;

    draw_sprite_row(screen_x + shift_x, screen_y + shift_y,
                    pixels, w, h, flip_h, row_offset, 1);
}

static void draw_rect_uniform_row(int screen_x, int screen_y,
                                  int w, int h, eadk_color_t color,
                                  int row_offset) {
    int y = screen_y + row_offset;
    if (y < 0 || y >= EADK_SCREEN_HEIGHT) return;
    if (row_offset < 0 || row_offset >= h) return;

    int draw_x = screen_x;
    int draw_w = w;
    if (draw_x < 0) { draw_w += draw_x; draw_x = 0; }
    if (draw_x + draw_w > EADK_SCREEN_WIDTH) draw_w = EADK_SCREEN_WIDTH - draw_x;
    if (draw_w <= 0) return;

    for (int x = 0; x < draw_w; x++) {
        line_buffer[draw_x + x] = color;
    }
}

static void find_sprite_bounds(const uint16_t *pixels, int w, int h,
                               int *min_x, int *min_y, int *max_x, int *max_y) {
    *min_x = w; *min_y = h; *max_x = -1; *max_y = -1;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint16_t p = pixels[y * w + x];
            if (p != 0x0001 && p != 0x0000) {
                if (x < *min_x) *min_x = x;
                if (x > *max_x) *max_x = x;
                if (y < *min_y) *min_y = y;
                if (y > *max_y) *max_y = y;
            }
        }
    }
    if (*min_x > *max_x) {
        *min_x = 0; *min_y = 0; *max_x = w - 1; *max_y = h - 1;
    }
}

// ---------- main ----------
int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    generate_background();
    rng_state = eadk_timing_millis();

    clear_world_mask();

#ifdef LEVEL_MAIN_SPAWN_X
    int spawn_tx = LEVEL_MAIN_SPAWN_X / TILE_SIZE;
    int spawn_ty = LEVEL_MAIN_SPAWN_Y / TILE_SIZE;
    while (!tile_solid(spawn_tx, spawn_ty) && spawn_ty < LEVEL_MAIN_ORIGIN_Y + LEVEL_MAIN_HEIGHT) {
        spawn_ty++;
    }
    float px = (float)LEVEL_MAIN_SPAWN_X;
    float py = (float)(spawn_ty * TILE_SIZE);
    if (tile_solid(spawn_tx, spawn_ty)) {
        py = (float)(spawn_ty * TILE_SIZE) - PLAYER_H;
    }
#else
    float px = (float)((LEVEL_MAIN_ORIGIN_X + 5) * TILE_SIZE);
    float py = (float)((LEVEL_MAIN_ORIGIN_Y + 5) * TILE_SIZE);
#endif

    scroll_x = px - EADK_SCREEN_WIDTH / 2.0f;
    scroll_y = py - EADK_SCREEN_HEIGHT / 2.0f;

    float move_vx = 0, move_vy = 0, nat_vx = 0;
    int air_timer = 0, wall_timer = 100, wall_slide = 0;
    int max_jumps = INITIAL_MAX_JUMPS;
    int jumps = max_jumps, facing_left = 0, color_i = 0;
    int jump_held_last = 0;

    float last_safe_x = px, last_safe_y = py;
    float last_safe_scroll_x = scroll_x, last_safe_scroll_y = scroll_y;

    static unsigned char entity_collected[LEVEL_MAIN_ENTITY_COUNT];
    for (int i = 0; i < LEVEL_MAIN_ENTITY_COUNT; i++) entity_collected[i] = 0;

    int idle_frame = 0, idle_timer = 0;
    int run_frame = 0, run_timer = 0;
    int spin_dir = 0;
    float player_rotation = 0.0f;
    float player_scale_y = 1.0f;

    int push_dir = 0, push_decay = 0;

    memset(blobs, 0, sizeof(blobs));
    clear_screen_mask();

    uint32_t start_time = eadk_timing_millis();
    uint32_t last_time = start_time;
    uint32_t accumulator = 0;
    const uint32_t dt_ms = 1000 / 60;
    uint32_t last_frame_time = 0;
    const uint32_t frame_ms = 1000 / 60;

    while (1) {
        uint32_t now = eadk_timing_millis();
        g_time_global = (now - start_time) / 1000.0f;
        float adj_time = g_time_global * 0.3f;
        float cos_t_0_07 = cosf(adj_time * 0.07f);
        float sin_t_0_05 = sinf(adj_time * 0.05f);
        float cos_t_0_05 = cosf(adj_time * 0.05f);

        if (now - last_frame_time < frame_ms) {
            eadk_keyboard_state_t keys = eadk_keyboard_scan();
            if (eadk_keyboard_key_down(keys, eadk_key_back)) break;
            continue;
        }
        last_frame_time = now;

        eadk_keyboard_state_t keys = eadk_keyboard_scan();
        if (eadk_keyboard_key_down(keys, eadk_key_back)) break;

        int left_held  = eadk_keyboard_key_down(keys, eadk_key_left);
        int right_held = eadk_keyboard_key_down(keys, eadk_key_right);
        int jump_held  = eadk_keyboard_key_down(keys, eadk_key_ok);
        int jump_pressed = jump_held && !jump_held_last;
        jump_held_last = jump_held;

        update_camera(px + PLAYER_W / 2.0f, py + PLAYER_H / 2.0f);

        now = eadk_timing_millis();
        uint32_t elapsed = now - last_time;
        last_time = now;
        accumulator += elapsed;

        int jump_press_pending = jump_pressed;
        float dt_sec = dt_ms / 1000.0f;

        while (accumulator >= dt_ms) {
            air_timer++;
            wall_timer++;

            player_scale_y += (1.0f - player_scale_y) / 3.5f;
            if (player_scale_y > 0.95f) player_scale_y = 1.0f;

            move_vy += GRAVITY_ACCEL;
            if (move_vy > GRAVITY_MAX) move_vy = GRAVITY_MAX;

            move_vx = normalize_f(move_vx, 0.15f, 0);
            nat_vx  = normalize_f(nat_vx, 0.07f, 0);

            if (nat_vx >= 0 && right_held) {
                move_vx += MOVE_ACCEL;
                if (move_vx > MOVE_MAX) move_vx = MOVE_MAX;
                facing_left = 0;
            }
            if (nat_vx <= 0 && left_held) {
                move_vx -= MOVE_ACCEL;
                if (move_vx < -MOVE_MAX) move_vx = -MOVE_MAX;
                facing_left = 1;
            }

            if (air_timer < 5 && (move_vx > 0.05f || move_vx < -0.05f)) {
                float angle = (move_vx > 0) ? -0.3f : -2.8f;
                angle += frand() * 0.5f - 0.25f;
                float force = frand() * 0.8f + 0.3f;
                spawn_blob(px + PLAYER_W/2, py + PLAYER_H * frand() + 4,
                           angle, force,
                           frand() * 2.0f + 1.5f,
                           0.05f, 0.2f);
            }

            if (jump_press_pending) {
                jump_press_pending = 0;
                if (!wall_slide) {
                    if (jumps > 0) {
                        move_vy = JUMP_VELOCITY;
                        jumps--;
                    }
                } else {
                    move_vy = WALL_JUMP_VY;
                    nat_vx = (float)wall_slide * WALL_JUMP_VX_KICK;
                    move_vx = 0;
                }
                if (air_timer > 4 || wall_slide) {
                    spin_dir = facing_left ? -1 : 1;
                }
                wall_slide = 0;
                for (int i = 0; i < 8; i++) {
                    float angle = frand() * 6.2832f;
                    float force = frand() * 5.0f + 3.0f;
                    spawn_blob(px + PLAYER_W/2, py + PLAYER_H/2,
                               angle, force,
                               frand() * 2.0f,
                               0.5f, 1.0f);
                }
            }

            if (spin_dir != 0) {
                player_rotation += (float)spin_dir * 16.0f;
            }

            float frame_mx = move_vx + nat_vx;
            float frame_my = move_vy;
            Collisions col = entity_move(&px, &py, frame_mx, frame_my);

            if (col.right) {
                push_dir = 1;
                push_decay = 10;
            } else if (col.left) {
                push_dir = -1;
                push_decay = 10;
            } else {
                if (push_decay > 0) push_decay--;
                else push_dir = 0;
            }

            clamp_player_to_view(&px, &py);

            // Jump-upgrade pickups
            {
                fRect player_rect = { px, py, (float)PLAYER_W, (float)PLAYER_H };
                for (int i = 0; i < LEVEL_MAIN_ENTITY_COUNT; i++) {
                    if (entity_collected[i]) continue;
                    if (LEVEL_MAIN_ENTITIES[i][2] != 1) continue;
                    fRect up_rect = { (float)LEVEL_MAIN_ENTITIES[i][0],
                                       (float)LEVEL_MAIN_ENTITIES[i][1],
                                       (float)jump_upgrade_width,
                                       (float)jump_upgrade_height };
                    if (rects_overlap_f(player_rect, up_rect)) {
                        entity_collected[i] = 1;
                        max_jumps++;
                        jumps = max_jumps;
                        for (int j = 0; j < 30; j++) {
                            float angle = frand() * 6.2832f;
                            float force = frand() * 6.7f + 4.5f;
                            spawn_blob(up_rect.x + up_rect.w/2, up_rect.y + up_rect.h/2,
                                       angle, force,
                                       0.0f, 0.2f, 0.5f);
                        }
                    }
                }
            }

            if (col.bottom || col.top) move_vy = 0;
            if (col.left || col.right) {
                if (air_timer > 4) {
                    if (move_vy > 0.2f) move_vy = 0.2f;
                    move_vx = 0;
                    spin_dir = 0;
                    player_rotation = 0.0f;
                    wall_timer = 0;
                    if (frame_mx > 0) wall_slide = 1;
                    if (frame_mx < 0) wall_slide = -1;
                }
            }
            if (wall_timer > 4) wall_slide = 0;

            if (col.bottom) {
                if (frame_my > 2.0f) {
                    player_scale_y = 0.2f;
                }
                air_timer = 0;
                spin_dir = 0;
                player_rotation = 0.0f;
                jumps = max_jumps;
                // Save safe position
                last_safe_x = px;
                last_safe_y = py - 1;
                last_safe_scroll_x = scroll_x;
                last_safe_scroll_y = scroll_y;
                if (frame_my > 2.0f) {
                    for (int i = 0; i < 15; i++) {
                        float angle = frand() * 6.2832f;
                        float force = frand() * 4.0f + 1.5f;
                        spawn_blob(px + PLAYER_W/2, py + PLAYER_H,
                                   angle, force,
                                   frand() * 2.0f,
                                   0.25f, 0.4f);
                    }
                }
            }

            // Death and respawn at last safe position
            if (py > scroll_y + EADK_SCREEN_HEIGHT + 48) {
                px = last_safe_x;
                py = last_safe_y - 1;
                scroll_x = last_safe_scroll_x;
                scroll_y = last_safe_scroll_y;

                move_vx = move_vy = nat_vx = 0;
                spin_dir = 0;
                player_rotation = 0.0f;
                player_scale_y = 1.0f;
                push_dir = 0; push_decay = 0;
            }

            accumulator -= dt_ms;
        }

        // Update blobs
        update_blobs(dt_sec);

        // Player base paint
        mark_paint((int)(px + PLAYER_W/2), (int)(py + PLAYER_H/2), BASE_PAINT_RADIUS);

        // Reveal around pillar and jump upgrades
        for (int i = 0; i < LEVEL_MAIN_ENTITY_COUNT; i++) {
            if (entity_collected[i]) continue;
            int ex = LEVEL_MAIN_ENTITIES[i][0];
            int ey = LEVEL_MAIN_ENTITIES[i][1];
            int etype = LEVEL_MAIN_ENTITIES[i][2];
            if (etype == 1) {
                mark_paint(ex + jump_upgrade_width/2, ey + jump_upgrade_height/2, UPGRADE_REVEAL_RADIUS);
            } else if (etype == 9) {
                mark_paint(ex + pillar_width/2, ey + pillar_height/2, PILLAR_REVEAL_RADIUS);
            }
        }

        // Animation ticks
        idle_timer++;
        if (idle_timer >= player_idle_frame_hold[idle_frame]) {
            idle_timer = 0;
            idle_frame = (idle_frame + 1) % 4;
        }
        run_timer++;
        if (run_timer >= 5) {
            run_timer = 0;
            run_frame = (run_frame + 1) % 8;
        }

        // Background cache
        int cam_x = (int)scroll_x;
        int cam_y = (int)scroll_y;
        generate_background_cache(cam_x, cam_y, adj_time, cos_t_0_07, sin_t_0_05, cos_t_0_05);

        // Build screen mask
        build_screen_mask(cam_x, cam_y);

        // Build draw list
        typedef struct {
            int16_t x, y, w, h, layer;
            const uint16_t *pixels;
            uint8_t flip;
            uint8_t is_foliage;
            float sway_seed;
        } DrawItem;

        #define MAX_DRAW_ITEMS (256 + LEVEL_MAIN_DECOR_COUNT + LEVEL_MAIN_FOLIAGE_COUNT + LEVEL_MAIN_ENTITY_COUNT + 8)
        static DrawItem draw_items[MAX_DRAW_ITEMS];
        int item_count = 0;

        VisibleEntity visible_entities[MAX_VISIBLE_ENTITIES];
        int visible_count = 0;

        // Tiles
        int tx0 = cam_x / TILE_SIZE - 1;
        int tx1 = (cam_x + EADK_SCREEN_WIDTH) / TILE_SIZE + 1;
        int ty0 = cam_y / TILE_SIZE - 1;
        int ty1 = (cam_y + EADK_SCREEN_HEIGHT) / TILE_SIZE + 1;

        for (int ty = ty0; ty <= ty1 && item_count < MAX_DRAW_ITEMS; ty++) {
            for (int tx = tx0; tx <= tx1; tx++) {
                int idx = tile_index(tx, ty);
                if (idx >= 0) {
                    int sx = tx * TILE_SIZE - cam_x;
                    int sy = ty * TILE_SIZE - cam_y + tile_y_offset[idx];
                    if (sx > -tile_w[idx] && sx < EADK_SCREEN_WIDTH &&
                        sy > -tile_h[idx] && sy < EADK_SCREEN_HEIGHT) {
                        DrawItem *it = &draw_items[item_count++];
                        it->x = sx; it->y = sy;
                        it->w = tile_w[idx]; it->h = tile_h[idx];
                        it->layer = tile_layer(tx, ty);
                        it->pixels = tile_pixels[idx];
                        it->flip = 0;
                        it->is_foliage = 0; it->sway_seed = 0.0f;
                        if (item_count >= MAX_DRAW_ITEMS) break;
                    }
                }
            }
        }

        // Decor
        for (int i = 0; i < LEVEL_MAIN_DECOR_COUNT && item_count < MAX_DRAW_ITEMS; i++) {
            int dx = LEVEL_MAIN_DECOR[i][0] - cam_x;
            int dy = LEVEL_MAIN_DECOR[i][1] - cam_y;
            int sheet = LEVEL_MAIN_DECOR[i][2];
            int tile = LEVEL_MAIN_DECOR[i][3];
            if (sheet >= 0 && sheet < 3 && tile >= 0 && tile < 3) {
                const uint16_t *pix = decor_sprites[sheet][tile];
                int w = decor_w[sheet][tile];
                int h = decor_h[sheet][tile];
                if (pix && w && h && dx > -w && dx < EADK_SCREEN_WIDTH &&
                    dy > -h && dy < EADK_SCREEN_HEIGHT) {
                    DrawItem *it = &draw_items[item_count++];
                    it->x = dx; it->y = dy; it->w = w; it->h = h;
                    it->layer = LEVEL_MAIN_DECOR[i][4];
                    it->pixels = pix; it->flip = 0;
                    it->is_foliage = 0; it->sway_seed = 0.0f;
                }
            }
        }

        // Foliage
        {
            float m_clock = g_time_global * 0.5f;
            static float foliage_seed[LEVEL_MAIN_FOLIAGE_COUNT];
            static int foliage_seed_ready = 0;
            if (!foliage_seed_ready) {
                for (int i = 0; i < LEVEL_MAIN_FOLIAGE_COUNT; i++) {
                    float ox = (float)LEVEL_MAIN_FOLIAGE[i][0];
                    float oy = (float)LEVEL_MAIN_FOLIAGE[i][1];
                    foliage_seed[i] = oy * ox + powf(ox + 10000000.0f, 1.2f);
                }
                foliage_seed_ready = 1;
            }

            for (int i = 0; i < LEVEL_MAIN_FOLIAGE_COUNT && item_count < MAX_DRAW_ITEMS; i++) {
                int orig_x = LEVEL_MAIN_FOLIAGE[i][0];
                int orig_y = LEVEL_MAIN_FOLIAGE[i][1];
                int sheet = LEVEL_MAIN_FOLIAGE[i][2];
                int tile = LEVEL_MAIN_FOLIAGE[i][3];
                if (sheet >= 0 && sheet < 5 && tile >= 0 && tile < 4) {
                    float seed = foliage_seed[i];
                    int sway_y = (int)(sinf(m_clock + 2.2f * seed) * 2.0f * 0.5f);

                    int fx = orig_x - cam_x;
                    int fy = orig_y + sway_y - cam_y;
                    const uint16_t *pix = foliage_sprites[sheet][tile];
                    int w = foliage_w[sheet][tile];
                    int h = foliage_h[sheet][tile];
                    if (pix && w && h && fx > -w - 4 && fx < EADK_SCREEN_WIDTH + 4 &&
                        fy > -h && fy < EADK_SCREEN_HEIGHT) {
                        DrawItem *it = &draw_items[item_count++];
                        it->x = fx; it->y = fy; it->w = w; it->h = h;
                        it->layer = LEVEL_MAIN_FOLIAGE[i][4];
                        it->pixels = pix; it->flip = 0;
                        it->is_foliage = 1; it->sway_seed = seed;
                    }
                }
            }
        }

        // Entities (pillar and jump upgrade as visible_entities)
        #define ANIM_TICKS_PER_FRAME 5
        static int entity_frame = 0;
        entity_frame++;
        int pillar_frame = max_jumps - 2;
        if (pillar_frame < 0) pillar_frame = 0;
        if (pillar_frame >= PILLAR_FRAMES) pillar_frame = PILLAR_FRAMES - 1;
        for (int i = 0; i < LEVEL_MAIN_ENTITY_COUNT; i++) {
            if (entity_collected[i]) continue;
            int ex = LEVEL_MAIN_ENTITIES[i][0] - cam_x;
            int ey = LEVEL_MAIN_ENTITIES[i][1] - cam_y;
            int etype = LEVEL_MAIN_ENTITIES[i][2];
            const uint16_t *pix = NULL;
            int w = 0, h = 0;
            switch (etype) {
                case 1: {
                    int frame_index = (entity_frame / ANIM_TICKS_PER_FRAME) % JUMP_UPGRADE_FRAMES;
                    pix = jump_upgrade_pixels[frame_index];
                    w = jump_upgrade_width; h = jump_upgrade_height;
                    break;
                }
                case 9:
                    pix = pillar_pixels[pillar_frame];
                    w = pillar_width; h = pillar_height;
                    break;
                default:
                    break;
            }
            if (pix && w && h && ex > -w && ex < EADK_SCREEN_WIDTH &&
                ey > -h && ey < EADK_SCREEN_HEIGHT) {
                if (visible_count < MAX_VISIBLE_ENTITIES) {
                    visible_entities[visible_count].x = ex;
                    visible_entities[visible_count].y = ey;
                    visible_entities[visible_count].w = w;
                    visible_entities[visible_count].h = h;
                    visible_entities[visible_count].pixels = pix;
                    visible_entities[visible_count].flip = 0;
                    visible_count++;
                }
            }
        }

        // Sort draw items by layer
        for (int i = 1; i < item_count; i++) {
            DrawItem key = draw_items[i];
            int j = i - 1;
            while (j >= 0 && draw_items[j].layer > key.layer) {
                draw_items[j + 1] = draw_items[j];
                j--;
            }
            draw_items[j + 1] = key;
        }

        int player_insert_idx = item_count;
        for (int i = 0; i < item_count; i++) {
            if (draw_items[i].layer >= -3) { player_insert_idx = i; break; }
        }

        // Player sprite computation
        const uint16_t *player_pixels = NULL;
        int pw = 0, ph = 0;
        int flip = facing_left;

        if (wall_slide != 0) {
            player_pixels = player_slide_pixels[0];
            pw = player_slide_width;
            ph = player_slide_height;
        } else if (air_timer > 4) {
            player_pixels = player_jump_pixels[0];
            pw = player_jump_width;
            ph = player_jump_height;
        } else {
            float abs_mvx = move_vx > 0 ? move_vx : -move_vx;
            if (abs_mvx > 0.1f) {
                player_pixels = player_run_pixels[run_frame];
                pw = player_run_width;
                ph = player_run_height;
            } else {
                player_pixels = player_idle_pixels[idle_frame];
                pw = player_idle_width;
                ph = player_idle_height;
            }
        }

        // Center sprite on collision box
        int offset_x = (PLAYER_W - pw) / 2;
int offset_y = PLAYER_H - ph;   // bottom‑align sprite with collision box
        int player_screen_x = (int)px - cam_x + offset_x;
        int player_screen_y = (int)py - cam_y + offset_y;

        float rot_cos_r = 1.0f, rot_sin_r = 0.0f;
        int rot_bbox_half = 0;
        int rot_center_x = 0, rot_center_y = 0;
        if (player_rotation != 0.0f) {
            float rot_rad = fmodf(player_rotation, 360.0f) * 0.0174532925f;
            rot_cos_r = cosf(rot_rad);
            rot_sin_r = sinf(rot_rad);
            float diag = sqrtf((float)(pw * pw + ph * ph));
            rot_bbox_half = (int)(diag * 0.5f + 1.0f);
            rot_center_x = player_screen_x + pw / 2;
            rot_center_y = player_screen_y + ph / 2;
        }

        // Render loop
        float foliage_m_clock = g_time_global * 0.5f;

        for (int y = 0; y < EADK_SCREEN_HEIGHT; y++) {
            // Background
            int cache_y = y >> 2;
            uint16_t *cache_row = bg_cache[cache_y];
            for (int x = 0; x < EADK_SCREEN_WIDTH; x++) {
                line_buffer[x] = cache_row[x >> 2];
            }

            // Draw background sprites (layers < -3)
            for (int i = 0; i < player_insert_idx; i++) {
                DrawItem *it = &draw_items[i];
                if (y >= it->y && y < it->y + it->h) {
                    if (it->is_foliage) {
                        draw_sprite_row_sway(it->x, it->y, it->pixels, it->w, it->h,
                                             it->flip, it->sway_seed, foliage_m_clock,
                                             y - it->y);
                    } else {
                        draw_sprite_row(it->x, it->y, it->pixels, it->w, it->h,
                                        it->flip, y - it->y, 1);
                    }
                }
            }

            // Draw player (before foreground layers)
            if (player_rotation != 0.0f) {
                draw_player_row_rotated(rot_center_x, rot_center_y,
                                        player_pixels, pw, ph, flip,
                                        rot_cos_r, rot_sin_r, rot_bbox_half, y);
            } else if (player_scale_y < 0.999f) {
                draw_player_row_squashed(player_screen_x, player_screen_y + ph,
                                         player_pixels, pw, ph, flip,
                                         player_scale_y, y);
            } else if (y >= player_screen_y && y < player_screen_y + ph) {
                draw_sprite_row(player_screen_x, player_screen_y,
                                player_pixels, pw, ph, flip, y - player_screen_y, 1);
            }

            // Draw visible entities (pillar, jump upgrade) also before foreground
            for (int i = 0; i < visible_count; i++) {
                VisibleEntity *ve = &visible_entities[i];
                if (y >= ve->y && y < ve->y + ve->h) {
                    draw_sprite_row(ve->x, ve->y, ve->pixels, ve->w, ve->h,
                                    ve->flip, y - ve->y, 1);
                }
            }

            // Draw foreground sprites (layers >= -3)
            for (int i = player_insert_idx; i < item_count; i++) {
                DrawItem *it = &draw_items[i];
                if (y >= it->y && y < it->y + it->h) {
                    if (it->is_foliage) {
                        draw_sprite_row_sway(it->x, it->y, it->pixels, it->w, it->h,
                                             it->flip, it->sway_seed, foliage_m_clock,
                                             y - it->y);
                    } else {
                        draw_sprite_row(it->x, it->y, it->pixels, it->w, it->h,
                                        it->flip, y - it->y, 1);
                    }
                }
            }

            // Apply mask blending
            for (int x = 0; x < EADK_SCREEN_WIDTH; x++) {
                uint8_t intensity = get_screen_intensity(x, y);
                if (intensity == 0) {
                    int border = 0;
                    if (x > 0 && get_screen_intensity(x - 1, y) > 0) border = 1;
                    else if (x < EADK_SCREEN_WIDTH - 1 && get_screen_intensity(x + 1, y) > 0) border = 1;
                    else if (y > 0 && get_screen_intensity(x, y - 1) > 0) border = 1;
                    else if (y < EADK_SCREEN_HEIGHT - 1 && get_screen_intensity(x, y + 1) > 0) border = 1;

                    uint16_t base_canvas = rgb_to_565(242, 242, 217);
                    if (border) base_canvas = rgb_to_565(204, 204, 178);

                    int h = (x * 31 + y * 17) + ((x * y) >> 3);
                    float n = ((h & 15) / 15.0f);
                    float paper = 0.9f + 0.1f * n;
                    line_buffer[x] = lerp_rgb565(base_canvas, rgb_to_565(255,255,255), paper);
                } else {
                    uint16_t scene = line_buffer[x];
                    if (intensity < 3) {
                        float t = intensity / 3.0f;
                        scene = lerp_rgb565(rgb_to_565(242, 242, 217), scene, t);
                    }
                    line_buffer[x] = scene;
                }
            }

            // Draw jump icons (black = available, white = used)
            {
                int icon_w = JUMP_ICON_WIDTH;
                int icon_h = JUMP_ICON_HEIGHT;
                int spacing = 2;
                for (int i = 0; i < max_jumps; i++) {
                    int iy = 5 + i * (icon_h + spacing);
                    if (y >= iy && y < iy + icon_h) {
                        int row = y - iy;
                        const uint16_t *src = &jump_icon_pixels[row * icon_w];
                        for (int x = 0; x < icon_w; x++) {
                            int draw_x = 5 + x;
                            if (draw_x < 0 || draw_x >= EADK_SCREEN_WIDTH) continue;
                            uint16_t pixel = src[x];
                            if (pixel != 0x0001 && pixel != 0x0000) {
                                if (i < jumps) {
                                    line_buffer[draw_x] = rgb_to_565(0, 0, 0);
                                } else {
                                    line_buffer[draw_x] = rgb_to_565(255, 255, 255);
                                }
                            }
                        }
                    }
                }
            }

            // Push line
            eadk_rect_t r = { 0, (uint16_t)y, EADK_SCREEN_WIDTH, 1 };
            eadk_display_push_rect(r, line_buffer);
        }
    }

    return 0;
}