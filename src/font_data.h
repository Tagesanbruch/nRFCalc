/*
 * Simple 5x7 bitmap font data
 * Each character is 5 pixels wide, 7 pixels tall
 * MSB is leftmost pixel, LSB is rightmost (only 5 bits used per byte)
 */

#ifndef FONT_DATA_H
#define FONT_DATA_H

#include <stdint.h>

#define FONT_WIDTH  5
#define FONT_HEIGHT 7

// Font data: 5x7 bitmap for ASCII 32-126
// Each character uses 7 bytes (one per row)
extern const uint8_t font_5x7[][FONT_HEIGHT];

/**
 * @brief Get font data for a character
 * @param ch ASCII character (32-126)
 * @return Pointer to 7-byte font data, or NULL if invalid
 */
const uint8_t* font_get_char_data(char ch);

#endif /* FONT_DATA_H */
