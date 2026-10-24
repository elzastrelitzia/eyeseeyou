#ifndef DISPLAY_WRAPPER_H
#define DISPLAY_WRAPPER_H

#include "config.h"

// ====================================================================================
// == SECTION 1 : Library inclusion and object instantiation                         ==
// ====================================================================================

#if defined(USE_U8G2)
    #include <U8g2lib.h>
    #include <Wire.h>
    // This constructor uses the board's default hardware I2C pins.
    // - Arduino Nano/Uno: A4 (SDA), A5 (SCL)
    // - ESP32: GPIO 21 (SDA), GPIO 22 (SCL)
    // - ESP8266: Use Wire.begin(SDA,SCL) in g_init_display()
    #if (SCREEN_HEIGHT == 32)
        U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);
    #else
        U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0); // For Arduino Nano 128x64
    #endif

    const int G_COLOR_WHITE = 1;
    const int G_COLOR_BLACK = 0;

#elif defined(USE_ADAFRUIT_SSD1306)
    #include <Wire.h>
    #include <Adafruit_GFX.h>
    #include <Adafruit_SSD1306.h>

    #define OLED_RESET -1
    Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

    const int G_COLOR_WHITE = SSD1306_WHITE;
    const int G_COLOR_BLACK = SSD1306_BLACK;

#else
    #error "No display library defined in config.h. Please choose one."
#endif

// ====================================================================================
// == SECTION 2 : Generic Wrapper Functions                                          ==
// ====================================================================================

inline void g_init_display() {
#if defined(BOARD_ESP8266_NODEMCU)
    // NodeMCU v2: D2 = GPIO4 (SDA), D1 = GPIO5 (SCL)
    Wire.begin(4, 5);
#elif defined(BOARD_ESP32)
    // Use default I2C pins unless overridden elsewhere
#endif
    // Arduino Nano/Uno uses default Wire pins (A4/A5)

#if defined(USE_U8G2)
    // Try common addresses; 0x3C is most common for SSD1306
    display.setI2CAddress(0x3C << 1);
    display.begin();
#elif defined(USE_ADAFRUIT_SSD1306)
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
#endif
}




inline void g_clear_display() {
#if defined(USE_U8G2)
    display.clearBuffer();
#elif defined(USE_ADAFRUIT_SSD1306)
    display.clearDisplay();
#endif
}

inline void g_update_display() {
#if defined(USE_U8G2)
    display.sendBuffer();
#elif defined(USE_ADAFRUIT_SSD1306)
    display.display();
#endif
}

inline void g_draw_filled_round_rect(int x, int y, int w, int h, int r, int color) {
#if defined(USE_U8G2)
    display.setDrawColor(color);
    display.drawRBox(x, y, w, h, r);
#elif defined(USE_ADAFRUIT_SSD1306)
    display.fillRoundRect(x, y, w, h, r, color);
#endif
}

inline void g_draw_filled_triangle(int x0, int y0, int x1, int y1, int x2, int y2, int color) {
#if defined(USE_U8G2)
    display.setDrawColor(color);
    display.drawTriangle(x0, y0, x1, y1, x2, y2);
#elif defined(USE_ADAFRUIT_SSD1306)
    display.fillTriangle(x0, y0, x1, y1, x2, y2, color);
#endif
}

#endif