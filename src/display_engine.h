/*
 * Display Engine - High-level display abstraction layer
 * 
 * This module provides a clean interface for drawing operations,
 * hiding the low-level display driver details from the application.
 */

#ifndef DISPLAY_ENGINE_H
#define DISPLAY_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize the display engine
 * @return 0 on success, negative error code on failure
 */
int display_engine_init(void);

/**
 * @brief Clear the entire display with specified color
 * @param color Background color (0 for default background)
 */
void display_engine_clear(uint32_t color);

/**
 * @brief Draw text at specified position with color
 * @param text Text string to draw
 * @param x X coordinate (pixels)
 * @param y Y coordinate (pixels) 
 * @param color Text color
 */
void display_engine_draw_text(const char *text, int x, int y, uint32_t color);

/**
 * @brief Draw large text at specified position with color
 * @param text Text string to draw
 * @param x X coordinate (pixels)
 * @param y Y coordinate (pixels) 
 * @param color Text color
 */
void display_engine_draw_text_large(const char *text, int x, int y, uint32_t color);

/**
 * @brief Draw a filled rectangle
 * @param x X coordinate (pixels)
 * @param y Y coordinate (pixels)
 * @param w Width (pixels)
 * @param h Height (pixels)
 * @param color Color value (format depends on display pixel format)
 */
void display_engine_draw_rect(int x, int y, int w, int h, uint32_t color);

/**
 * @brief Fill a rectangle with specified color
 * @param x X coordinate (pixels)
 * @param y Y coordinate (pixels)
 * @param w Width (pixels)
 * @param h Height (pixels)
 * @param color Fill color
 */
void display_engine_fill_rect(int x, int y, int w, int h, uint32_t color);

/**
 * @brief Set a single pixel
 * @param x X coordinate (pixels)
 * @param y Y coordinate (pixels)
 * @param color Pixel color
 */
void display_engine_set_pixel(int x, int y, uint32_t color);

/**
 * @brief Present/flush all drawing operations to the display
 */
void display_engine_present(void);

/**
 * @brief Update/present the display
 */
void display_engine_update(void);

/**
 * @brief Get display width in pixels
 * @return Display width
 */
int display_engine_get_width(void);

/**
 * @brief Get display height in pixels  
 * @return Display height
 */
int display_engine_get_height(void);

/**
 * @brief Turn display on/off
 * @param on true to turn on, false to turn off
 */
void display_engine_set_blanking(bool on);

#endif /* DISPLAY_ENGINE_H */
