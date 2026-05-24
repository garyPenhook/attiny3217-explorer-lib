#pragma once

#include <stdint.h>

// SSD1306 OLED driver for the Curiosity Nano Explorer display.
namespace oled {

// Explorer OLED 7-bit TWI address.
inline constexpr uint8_t address = 0x3du;

// Display geometry for the Explorer's 128x64 SSD1306 panel.
inline constexpr uint8_t width = 128u;
inline constexpr uint8_t height = 64u;
inline constexpr uint8_t page_count = height / 8u;

// Send the SSD1306 initialization sequence and clear the panel.
bool init();

// Clear all display RAM pages.
bool clear();

// Write a NUL-terminated text string to one 8-pixel-high display page.
bool write_text_page(uint8_t page, const char* text);

// Render the foreground telemetry summary on page 0.
bool write_status(uint16_t millivolts, bool twi_ok);

}  // namespace oled
