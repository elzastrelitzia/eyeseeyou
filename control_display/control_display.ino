/*
 * Main unified file for eye animations.
 * Library selection is done in "config.h".
*/
#include "display_wrapper.h" // Display abstraction layer

// --- Constants ---
const long BAUD_RATE = 115200;

// Use an enum for animation indexes to avoid "magic numbers"
enum Animation {
  WAKEUP,
  RESET,
  MOVE_RIGHT_BIG,
  MOVE_LEFT_BIG,
  BLINK_LONG,
  BLINK_SHORT,
  HAPPY,
  SLEEP,
  SACCADE_RANDOM,
  MAX_ANIMATIONS // Helper to get the count of animations
};

// Idle timings
const unsigned long IDLE_BEFORE_SLEEP_MS = 10000;
const unsigned long SLEEP_DURATION_MS = 10000;
const unsigned long IDLE_MOVE_GAP_MS = 1280;
unsigned long last_activity = 0;
unsigned long last_idle_move = 0;
int idle_direction = 1;
bool sleeping = false;

#ifdef USE_WEB_CONTROL
  #include <ESPAsyncTCP.h>
  #include <ESPAsyncWebServer.h>
  #include <ArduinoJson.h>
  #include <FS.h>

  AsyncWebServer server(80);
  int pending_web_anim = -1;
  bool web_idle_enabled = true;
  int web_speed = 12;
#endif

// snowflake animation constants
#define NUM_FLAKES 20
#define WIND_DX 1.2f
#define WIND_DY 0.4f

struct Flake {
  float x, y;
  float vx, vy;
  float wobble;
  float phase;
  uint8_t size;
};

Flake flakes[NUM_FLAKES];

void spawnFlake(Flake &f, bool randomPos) {
  if (randomPos) {
    f.x = random(0, SCREEN_WIDTH);
    f.y = random(0, SCREEN_HEIGHT);
  } else {
    if (WIND_DX > 0 && random(0, 2) == 0) {
      f.x = -random(1, 10);
      f.y = random(0, SCREEN_HEIGHT);
    } else if (WIND_DX < 0 && random(0, 2) == 0) {
      f.x = SCREEN_WIDTH + random(1, 10);
      f.y = random(0, SCREEN_HEIGHT);
    } else {
      f.x = random(0, SCREEN_WIDTH);
      f.y = -random(1, 10);
    }
  }
  float speedMul = 0.7 + random(0, 60) / 100.0;
  f.vx = WIND_DX * speedMul;
  f.vy = 0.6 * speedMul + WIND_DY;
  f.wobble = 0.1 + random(0, 20) / 100.0;
  f.phase = random(0, 628) / 100.0;
  f.size = random(1, 4);
}

void drawFlake(const Flake &f) {
  int x = (int)f.x;
  int y = (int)f.y;
  switch (f.size) {
    case 1:
      g_draw_pixel(x, y, G_COLOR_WHITE);
      break;
    case 2:
      g_draw_pixel(x, y, G_COLOR_WHITE);
      g_draw_pixel(x - 1, y, G_COLOR_WHITE);
      g_draw_pixel(x + 1, y, G_COLOR_WHITE);
      g_draw_pixel(x, y - 1, G_COLOR_WHITE);
      g_draw_pixel(x, y + 1, G_COLOR_WHITE);
      break;
    case 3:
      g_draw_line(x - 2, y, x + 2, y, G_COLOR_WHITE);
      g_draw_line(x, y - 2, x, y + 2, G_COLOR_WHITE);
      g_draw_line(x - 1, y - 1, x + 1, y + 1, G_COLOR_WHITE);
      g_draw_line(x - 1, y + 1, x + 1, y - 1, G_COLOR_WHITE);
      break;
  }
}

void show_snow_animation(unsigned long duration_ms = 8000) {
  for (int i = 0; i < NUM_FLAKES; i++) spawnFlake(flakes[i], true);

  unsigned long start = millis();
  while (millis() - start < duration_ms) {
    g_clear_display();
    for (int i = 0; i < NUM_FLAKES; i++) {
      Flake &f = flakes[i];
      f.phase += 0.1;
      f.x += f.vx + sin(f.phase) * f.wobble;
      f.y += f.vy;

      bool offBottom = f.y >= SCREEN_HEIGHT;
      bool offRight  = f.x >= SCREEN_WIDTH && WIND_DX > 0;
      bool offLeft   = f.x < 0 && WIND_DX < 0;
      if (offBottom || offRight || offLeft) spawnFlake(f, false);

      drawFlake(f);
    }
    g_update_display();
    delay(16);
  }
}

// Bitmap animation (alternates with snow)
const uint8_t PROGMEM frame0[512] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xe0,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfb, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xc0, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x1f,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x03, 0xf7, 0x0f, 0xe1, 0x18, 0x8c, 0xc3, 0xff, 0xe3, 0x0c, 0xef, 0xd8, 0x60, 0x00, 0x00, 0x00,
  0x03, 0x12, 0x08, 0xc3, 0x19, 0x88, 0xc2, 0x31, 0x23, 0x0c, 0x88, 0x5c, 0x20, 0x00, 0x00, 0x10,
  0x02, 0x06, 0x01, 0x85, 0x99, 0x99, 0x42, 0x03, 0x05, 0x09, 0x08, 0x16, 0x20, 0x03, 0xe1, 0xc0,
  0x03, 0xe6, 0x03, 0x05, 0x8a, 0xd1, 0x63, 0x83, 0x0d, 0x0e, 0x0f, 0x16, 0x40, 0x1c, 0xff, 0x00,
  0x02, 0x46, 0x02, 0x0d, 0x8a, 0xd2, 0x60, 0xe3, 0x09, 0x8a, 0x09, 0x13, 0x40, 0x01, 0xf8, 0x00,
  0x06, 0x04, 0x04, 0x0e, 0x8c, 0x67, 0xe0, 0x62, 0x1d, 0x9b, 0x18, 0x11, 0xc0, 0x03, 0xc0, 0x00,
  0x06, 0x14, 0x28, 0x50, 0xcc, 0x64, 0x24, 0x62, 0x10, 0x99, 0x18, 0x90, 0xc0, 0x07, 0x00, 0x00,
  0x0f, 0xef, 0xdf, 0xf0, 0xc8, 0x4c, 0x77, 0xc7, 0x31, 0xd9, 0xff, 0xb0, 0xc0, 0x1c, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x18, 0x00, 0x80,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x38, 0x07, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0xff, 0xfc, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x8f, 0xc0, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x8e, 0x20, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x0d, 0xc0, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0xfe, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xc3, 0xc0, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0xf8, 0x03, 0x00, 0x00
};
// Variant 2 (2s)
const uint8_t PROGMEM frame1[512] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe0, 0x1f,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x04, 0x00,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x3f, 0xfe,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf3, 0xff, 0xe0,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xfc, 0x08, 0xf0, 0x1e, 0xe7, 0x73, 0x3c, 0x00, 0x1c, 0xf3, 0x10, 0x27, 0x9f, 0xff, 0xff, 0xff,
  0xfc, 0xed, 0xf7, 0x3c, 0xe6, 0x77, 0x3d, 0xce, 0xdc, 0xf3, 0x77, 0xa3, 0xdf, 0xff, 0xff, 0xef,
  0xfd, 0xf9, 0xfe, 0x7a, 0x66, 0x66, 0xbd, 0xfc, 0xfa, 0xf6, 0xf7, 0xe9, 0xdf, 0xfc, 0x1e, 0x3f,
  0xfc, 0x19, 0xfc, 0xfa, 0x75, 0x2e, 0x9c, 0x7c, 0xf2, 0xf1, 0xf0, 0xe9, 0xbf, 0xe3, 0x00, 0xff,
  0xfd, 0xb9, 0xfd, 0xf2, 0x75, 0x2d, 0x9f, 0x1c, 0xf6, 0x75, 0xf6, 0xec, 0xbf, 0xfe, 0x07, 0xff,
  0xf9, 0xfb, 0xfb, 0xf1, 0x73, 0x98, 0x1f, 0x9d, 0xe2, 0x64, 0xe7, 0xee, 0x3f, 0xfc, 0x3f, 0xff,
  0xf9, 0xeb, 0xd7, 0xaf, 0x33, 0x9b, 0xdb, 0x9d, 0xef, 0x66, 0xe7, 0x6f, 0x3f, 0xf8, 0xff, 0xff,
  0xf0, 0x10, 0x20, 0x0f, 0x37, 0xb3, 0x88, 0x38, 0xce, 0x26, 0x00, 0x4f, 0x3f, 0xe3, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xe7, 0xff, 0x7f,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0xc7, 0xf8, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x00, 0x03, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x70, 0x3f, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x71, 0xdf, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc1, 0xf2, 0x3f, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf8, 0x01, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x3c, 0x3f, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};
// Author :- Ashish ( ashish_y_794 )
// Website : https://www.oledanimationmaker.com/

void show_bitmap_animation(unsigned long duration_ms = 5000) {
  // Variant 1: 2 seconds
  unsigned long start = millis();
  while (millis() - start < 2000) {
    g_clear_display();
    g_draw_bitmap(0, 0, frame0, 128, 32, G_COLOR_WHITE);
    g_update_display();
    delay(100);
  }
  // Variant 2: 2 seconds
  start = millis();
  while (millis() - start < 2000) {
    g_clear_display();
    g_draw_bitmap(0, 0, frame1, 128, 32, G_COLOR_WHITE);
    g_update_display();
    delay(100);
  }
}

// --- Structs for better state management ---

// Structure to hold the reference state of the eyes
struct EyeState {
  int height;
  int width;
  int x;
  int y;
};

// Eye dimensions scaled by screen size
#if (SCREEN_HEIGHT == 32)
  // Optimized for 128x32 displays
  const int REF_EYE_HEIGHT = 24;
  const int REF_EYE_WIDTH  = 24;
  const int REF_SPACE_BETWEEN_EYE = 6;
  const int REF_CORNER_RADIUS = 8;
#else
  // Default for 128x64 displays
  const int REF_EYE_HEIGHT = 40;
  const int REF_EYE_WIDTH  = 40;
  const int REF_SPACE_BETWEEN_EYE = 10;
  const int REF_CORNER_RADIUS = 10;
#endif

// Current state of the eyes
EyeState left_eye, right_eye;
int corner_radius = REF_CORNER_RADIUS;

int calculate_safe_radius(int r, int w, int h) {
    // Adhere to U8g2's drawRBox constraints: r must be smaller than half of the width/height.
    // For integer arithmetic, this means r <= (dimension / 2) - 1.
    if (w < 2 * (r + 1)) {
        r = (w / 2) - 1;
    }
    if (h < 2 * (r + 1)) {
        r = (h / 2) - 1;
    }
    return (r < 0) ? 0 : r; // Ensure radius is not negative
}

void draw_eyes()
{
    int r_left = calculate_safe_radius(corner_radius, left_eye.width, left_eye.height);
    int x_left = int(left_eye.x - left_eye.width / 2);
    int y_left = int(left_eye.y - left_eye.height / 2);
    g_draw_filled_round_rect(x_left, y_left, left_eye.width, left_eye.height, r_left, G_COLOR_WHITE);

    int r_right = calculate_safe_radius(corner_radius, right_eye.width, right_eye.height);
    int x_right = int(right_eye.x - right_eye.width / 2);
    int y_right = int(right_eye.y - right_eye.height / 2);
    g_draw_filled_round_rect(x_right, y_right, right_eye.width, right_eye.height, r_right, G_COLOR_WHITE);
}

void draw_frame() {
    g_clear_display();
    draw_eyes();
    g_update_display();
}


void reset_eyes(bool update=true)
{
  //move eyes to the center of the display, defined by SCREEN_WIDTH, SCREEN_HEIGHT
  left_eye.height = REF_EYE_HEIGHT;
  left_eye.width = REF_EYE_WIDTH;
  right_eye.height = REF_EYE_HEIGHT;
  right_eye.width = REF_EYE_WIDTH;

  left_eye.x = SCREEN_WIDTH / 2 - REF_EYE_WIDTH / 2 - REF_SPACE_BETWEEN_EYE / 2;
  left_eye.y = SCREEN_HEIGHT / 2;
  right_eye.x = SCREEN_WIDTH / 2 + REF_EYE_WIDTH / 2 + REF_SPACE_BETWEEN_EYE / 2;
  right_eye.y = SCREEN_HEIGHT / 2;

  corner_radius = REF_CORNER_RADIUS;

  if (update) {
    draw_frame();
  }
}

void blink(int speed=12)
{
#if (SCREEN_HEIGHT == 32)
  // 3 steps must close the eye: 3 * speed ~= REF_EYE_HEIGHT - 2
  speed = (REF_EYE_HEIGHT - 2) / 3;
  if (speed < 2) speed = 2;
#endif
  reset_eyes(false); // Ensure starting from a clean state
  for(int i=0;i<3;i++)
  {
    left_eye.height -= speed;
    right_eye.height -= speed;

     // Dynamically adjust corner_radius during blink
    int current_h = left_eye.height;
    int min_h_blink = (SCREEN_HEIGHT == 32) ? 2 : 4;
    int mapped_radius = map(current_h, min_h_blink, REF_EYE_HEIGHT, 1, REF_CORNER_RADIUS);
    corner_radius = min(mapped_radius, current_h / 2);

    left_eye.width += (SCREEN_HEIGHT == 32) ? 2 : 3;
    right_eye.width += (SCREEN_HEIGHT == 32) ? 2 : 3;
    draw_frame();
    delay(1);
  }
  for(int i=0;i<3;i++)
  {
    left_eye.height += speed;
    right_eye.height += speed;

    // Dynamically adjust corner_radius during blink
    int current_h = left_eye.height;
    int min_h_blink = (SCREEN_HEIGHT == 32) ? 2 : 4;
    int mapped_radius = map(current_h, min_h_blink, REF_EYE_HEIGHT, 1, REF_CORNER_RADIUS);
    corner_radius = min(mapped_radius, current_h / 2);

    left_eye.width -= (SCREEN_HEIGHT == 32) ? 2 : 3;
    right_eye.width -= (SCREEN_HEIGHT == 32) ? 2 : 3;
    draw_frame();
    delay(1);
  }
  reset_eyes();
}



void sleep()
{
  #if (SCREEN_HEIGHT == 32)
    for(int h = REF_EYE_HEIGHT; h >= 1; h -= 1)
  #else
    for(int h = REF_EYE_HEIGHT; h >= 2; h -= 2)
  #endif
    {
      left_eye.height = h;
      right_eye.height = h;
      int min_h = (SCREEN_HEIGHT == 32) ? 1 : 2;
      int mapped_radius = map(h, min_h, REF_EYE_HEIGHT, 1, REF_CORNER_RADIUS);
      corner_radius = min(mapped_radius, h / 2);
      left_eye.width = REF_EYE_WIDTH;
      right_eye.width = REF_EYE_WIDTH;
      draw_frame();
    }
}
void wakeup()
{
  reset_eyes(false);

#if (SCREEN_HEIGHT == 32)
  for(int h = 1; h <= REF_EYE_HEIGHT; h += 1)
#else
  for(int h = 2; h <= REF_EYE_HEIGHT; h += 2)
#endif
  {
    left_eye.height = h;
    right_eye.height = h;
    int min_h_wake = (SCREEN_HEIGHT == 32) ? 1 : 2;
    int mapped_radius = map(h, min_h_wake, REF_EYE_HEIGHT, 1, REF_CORNER_RADIUS);
    // The corner radius can't be more than half the height, otherwise it distorts.
    corner_radius = min(mapped_radius, h / 2);
    draw_frame();
  }


}


void happy_eye()
{
  reset_eyes(true); // Draw the initial eyes to the screen
  int offset = REF_EYE_HEIGHT / 2;
#if (SCREEN_HEIGHT == 32)
  for(int i=0;i<5;i++)
#else
  for(int i=0;i<10;i++)
#endif
  {
    // Draw the "happy" lower eyelid shape for both eyes
    g_draw_filled_triangle(left_eye.x - left_eye.width / 2 - 1, left_eye.y + offset, left_eye.x + left_eye.width / 2 + 1, left_eye.y + (SCREEN_HEIGHT == 32 ? 2 : 5) + offset, left_eye.x - left_eye.width / 2 - 1, left_eye.y + left_eye.height + offset, G_COLOR_BLACK);
    g_draw_filled_triangle(right_eye.x + right_eye.width / 2 + 1, right_eye.y + offset, right_eye.x - right_eye.width / 2 - 2, right_eye.y + (SCREEN_HEIGHT == 32 ? 2 : 5) + offset, right_eye.x + right_eye.width / 2 + 1, right_eye.y + right_eye.height + offset, G_COLOR_BLACK);
    offset -= (SCREEN_HEIGHT == 32) ? 1 : 2;
    g_update_display();
    delay(1);
  }
  delay(1000);
  reset_eyes();
}

void saccade(int direction_x, int direction_y)
{
  //quick movement of the eye, no size change. stay at position after movement, will not move back,  call again with opposite direction
  //direction == -1 :  move left
  //direction == 1 :  move right

#if (SCREEN_HEIGHT == 32)
  const int MOVEMENT_AMPLITUDE_X = 3;
  const int MOVEMENT_AMPLITUDE_Y = 2;
  const int BLINK_AMPLITUDE = 2;
#else
  const int MOVEMENT_AMPLITUDE_X = 8;
  const int MOVEMENT_AMPLITUDE_Y = 6;
  const int BLINK_AMPLITUDE = 8;
#endif

  // Animate the saccade in a single loop for simplicity
  for(int i = 1; i <= 2; i++)
  {
    left_eye.x += MOVEMENT_AMPLITUDE_X * direction_x;
    right_eye.x += MOVEMENT_AMPLITUDE_X * direction_x;
    left_eye.y += MOVEMENT_AMPLITUDE_Y * direction_y;
    right_eye.y += MOVEMENT_AMPLITUDE_Y * direction_y;

    // Eyes narrow during the first half of movement, then widen back
    int height_change = (i == 1) ? -BLINK_AMPLITUDE : BLINK_AMPLITUDE;
    right_eye.height += height_change;
    left_eye.height += height_change;

    draw_frame();
    delay(1);
  }

}

void move_right_big_eye()
{
  move_big_eye(1);
}
void move_left_big_eye()
{
  move_big_eye(-1);
}

void move_big_eye(int direction)
{
  //direction == -1 :  move left
  //direction == 1 :  move right
  reset_eyes(false);

#if (SCREEN_HEIGHT == 32)
  const int OVERSIZE_AMOUNT = 0;
  const int MOVEMENT_AMPLITUDE = 1;
  const int BLINK_AMPLITUDE = 2;
#else
  const int OVERSIZE_AMOUNT = 1;
  const int MOVEMENT_AMPLITUDE = 2;
  const int BLINK_AMPLITUDE = 5;
#endif

  // --- Move eyes out ---
  for(int i=0;i<3;i++)
  {
    left_eye.x += MOVEMENT_AMPLITUDE * direction;
    right_eye.x += MOVEMENT_AMPLITUDE * direction;
    right_eye.height -= BLINK_AMPLITUDE;
    left_eye.height -= BLINK_AMPLITUDE;

    if (OVERSIZE_AMOUNT != 0) {
      EyeState* target_eye = (direction > 0) ? &right_eye : &left_eye;
      target_eye->height += OVERSIZE_AMOUNT;
      target_eye->width += OVERSIZE_AMOUNT;
    }

    draw_frame();
    delay(1);
  }
  for(int i=0;i<3;i++)
  {
    left_eye.x += MOVEMENT_AMPLITUDE * direction;
    right_eye.x += MOVEMENT_AMPLITUDE * direction;
    right_eye.height += BLINK_AMPLITUDE;
    left_eye.height += BLINK_AMPLITUDE;

    if (OVERSIZE_AMOUNT != 0) {
      EyeState* target_eye = (direction > 0) ? &right_eye : &left_eye;
      target_eye->height += OVERSIZE_AMOUNT;
      target_eye->width += OVERSIZE_AMOUNT;
    }

    draw_frame();
    delay(1);
  }

  delay(1000);

  // --- Move eyes back ---
  for(int i=0;i<3;i++)
  {
    left_eye.x -= MOVEMENT_AMPLITUDE * direction;
    right_eye.x -= MOVEMENT_AMPLITUDE * direction;
    right_eye.height -= BLINK_AMPLITUDE;
    left_eye.height -= BLINK_AMPLITUDE;

    if (OVERSIZE_AMOUNT != 0) {
      EyeState* target_eye = (direction > 0) ? &right_eye : &left_eye;
      target_eye->height -= OVERSIZE_AMOUNT;
      target_eye->width -= OVERSIZE_AMOUNT;
    }

    draw_frame();
    delay(1);
  }
  for(int i=0;i<3;i++)
  {
    left_eye.x -= MOVEMENT_AMPLITUDE * direction;
    right_eye.x -= MOVEMENT_AMPLITUDE * direction;
    right_eye.height += BLINK_AMPLITUDE;
    left_eye.height += BLINK_AMPLITUDE;

    if (OVERSIZE_AMOUNT != 0) {
      EyeState* target_eye = (direction > 0) ? &right_eye : &left_eye;
      target_eye->height -= OVERSIZE_AMOUNT;
      target_eye->width -= OVERSIZE_AMOUNT;
    }

    draw_frame();
    delay(1);
  }

  reset_eyes();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(BAUD_RATE);
  g_init_display();

  reset_eyes();
  // Send the "READY" signal BEFORE the delay to allow the host to sync up.
  Serial.println("READY");
  delay(3000);

  last_activity = millis();
}



void launch_animation_with_index(int animation_index)
{

  switch(animation_index)
      {
        case WAKEUP:
          wakeup();
          break;
        case RESET:
          reset_eyes(true);
          break;
        case MOVE_RIGHT_BIG:
          move_right_big_eye();
          break;
        case MOVE_LEFT_BIG:
          move_left_big_eye();
          break;
        case BLINK_LONG:
          blink(12);
          delay(1000);
          break;
        case BLINK_SHORT:
          blink(12);
          break;
        case HAPPY:
          happy_eye();
          break;
        case SLEEP:
          sleep();
          break;
        case SACCADE_RANDOM:
          reset_eyes(true);
#if (SCREEN_HEIGHT == 32)
          for(int i=0;i<10;i++)
#else
          for(int i=0;i<20;i++)
#endif
          {
            int dir_x = random(-1, 2);
            int dir_y = random(-1, 2);
            saccade(dir_x,dir_y);
            delay(1);
            saccade(-dir_x,-dir_y);
            delay(1);
          }
          break;
        default:
          // Do nothing for unknown animations
          break;

      }
}



void loop() {

  //send "Ax" for animation x  (ex. A2 will launch animation 2)
  if (Serial.available() > 0) {
    // Peek at the first character to see if it's a command start.
    char firstChar = Serial.peek();
    if (firstChar == 'A' || firstChar == 'a') {
      // Read the command character 'A'
      Serial.read();
      // Read the integer part of the command. parseInt() is non-blocking
      int anim_index = Serial.parseInt();

      if (anim_index < MAX_ANIMATIONS) {
        launch_animation_with_index(anim_index);
        Serial.print("Executed animation: ");
        Serial.println(anim_index);
      }
    } else {
      // Discard any unexpected characters in the buffer to prevent them from piling up.
      Serial.read();
    }
    // Any serial traffic counts as activity
    last_activity = millis();
    sleeping = false;
  }

  unsigned long now = millis();

  if (!sleeping && now - last_activity >= IDLE_BEFORE_SLEEP_MS) {
    sleep();
    sleeping = true;
    last_activity = now;
  } else if (sleeping && now - last_activity >= SLEEP_DURATION_MS) {
    static bool show_snow_next = true;
    if (show_snow_next) {
      show_snow_animation(5000);
    } else {
      show_bitmap_animation(5000);
    }
    show_snow_next = !show_snow_next;
    wakeup();
    move_right_big_eye();
    move_left_big_eye();
    blink();
    blink();
    reset_eyes();
    sleeping = false;
    last_activity = millis();
  } else if (!sleeping && now - last_idle_move >= IDLE_MOVE_GAP_MS) {
    move_big_eye(idle_direction);
    idle_direction = -idle_direction;
    last_idle_move = millis();
  }
}
