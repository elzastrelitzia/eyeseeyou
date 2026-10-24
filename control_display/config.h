#ifndef CONFIG_H
#define CONFIG_H

// --- BOARD SELECTION ---
// Uncomment ONLY ONE of the following lines to choose your board.
#define BOARD_ESP8266_NODEMCU
//#define BOARD_ARDUINO_NANO
//#define BOARD_ESP32

// --- LIBRARY SELECTION ---
// Uncomment ONLY ONE of the following lines to choose your display driver.
#define USE_ADAFRUIT_SSD1306
//#define USE_U8G2

// --- Common Configuration ---
#if defined(BOARD_ESP8266_NODEMCU) || defined(BOARD_ESP32)
  #define SCREEN_WIDTH  128
  #define SCREEN_HEIGHT 32
#else // Default to Arduino Nano
  #define SCREEN_WIDTH  128
  #define SCREEN_HEIGHT 64
#endif

#endif