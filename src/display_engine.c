/*
 * Display Engine Implementation
 * 
 * This module implements high-level display operations using the Zephyr display driver.
 * It abstracts away pixel format details and provides simple drawing primitives.
 */

#include "display_engine.h"
#include "font_data.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(display_engine, LOG_LEVEL_INF);

// Internal state
static const struct device *display_dev = NULL;
static struct display_capabilities capabilities;
static struct display_buffer_descriptor buf_desc;
static uint8_t frame_buffer[320 * 240 * 4] __aligned(4); // Static allocation for max size
static size_t frame_buffer_size = 0;
static uint32_t bg_color = 0;
static uint32_t fg_color = 0;

// Function pointer for pixel format specific operations
static void (*fill_buffer_fnc)(uint32_t color, uint8_t *buf, size_t buf_size) = NULL;

// Pixel format specific fill functions
static void fill_buffer_argb8888(uint32_t color, uint8_t *buf, size_t buf_size)
{
    for (size_t idx = 0; idx < buf_size; idx += 4) {
        *((uint32_t *)(buf + idx)) = color;
    }
}

static void fill_buffer_rgb888(uint32_t color, uint8_t *buf, size_t buf_size)
{
    for (size_t idx = 0; idx < buf_size; idx += 3) {
        *(buf + idx + 0) = (color >> 16) & 0xFF;
        *(buf + idx + 1) = (color >> 8) & 0xFF;
        *(buf + idx + 2) = (color >> 0) & 0xFF;
    }
}

static void fill_buffer_rgb565(uint32_t color, uint8_t *buf, size_t buf_size)
{
    uint16_t color16 = (uint16_t)color;
    for (size_t idx = 0; idx < buf_size; idx += 2) {
        *(buf + idx + 0) = (color16 >> 8) & 0xFF;
        *(buf + idx + 1) = (color16 >> 0) & 0xFF;
    }
}

static void fill_buffer_bgr565(uint32_t color, uint8_t *buf, size_t buf_size)
{
    uint16_t color16 = (uint16_t)color;
    for (size_t idx = 0; idx < buf_size; idx += 2) {
        *(uint16_t *)(buf + idx) = color16;
    }
}

static void fill_buffer_mono01(uint32_t color, uint8_t *buf, size_t buf_size)
{
    uint8_t fill_value = (color != 0) ? 0xFF : 0x00;
    memset(buf, fill_value, buf_size);
}

static void fill_buffer_mono10(uint32_t color, uint8_t *buf, size_t buf_size)
{
    uint8_t fill_value = (color != 0) ? 0x00 : 0xFF;
    memset(buf, fill_value, buf_size);
}

// Convert RGB888 color to current pixel format
static uint32_t convert_color(uint8_t r, uint8_t g, uint8_t b)
{
    switch (capabilities.current_pixel_format) {
    case PIXEL_FORMAT_ARGB_8888:
        return 0xFF000000 | (r << 16) | (g << 8) | b;
    case PIXEL_FORMAT_RGB_888:
        return (r << 16) | (g << 8) | b;
    case PIXEL_FORMAT_RGB_565:
    case PIXEL_FORMAT_BGR_565:
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    case PIXEL_FORMAT_MONO01:
    case PIXEL_FORMAT_MONO10:
        // Convert to grayscale using standard formula
        return ((r * 299 + g * 587 + b * 114) / 1000) > 128 ? 1 : 0;
    default:
        return 0;
    }
}

int display_engine_init(void)
{
    // Get display device
    display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(display_dev)) {
        LOG_ERR("Display device not ready");
        return -ENODEV;
    }

    // Get display capabilities
    display_get_capabilities(display_dev, &capabilities);
    LOG_INF("Display: %dx%d, format=%d", 
            capabilities.x_resolution, 
            capabilities.y_resolution,
            capabilities.current_pixel_format);

    // Calculate frame buffer size
    frame_buffer_size = capabilities.x_resolution * capabilities.y_resolution;

    // Setup pixel format specific parameters
    LOG_INF("Setting up pixel format: %d", capabilities.current_pixel_format);
    switch (capabilities.current_pixel_format) {
    case PIXEL_FORMAT_ARGB_8888:
        fill_buffer_fnc = fill_buffer_argb8888;
        frame_buffer_size *= 4;
        bg_color = convert_color(173, 216, 230);  // Light blue background
        fg_color = convert_color(0, 0, 0);        // Black foreground
        break;
    case PIXEL_FORMAT_RGB_888:
        fill_buffer_fnc = fill_buffer_rgb888;
        frame_buffer_size *= 3;
        bg_color = convert_color(173, 216, 230);  // Light blue background
        fg_color = convert_color(0, 0, 0);        // Black foreground
        break;
    case PIXEL_FORMAT_RGB_565:
        fill_buffer_fnc = fill_buffer_rgb565;
        frame_buffer_size *= 2;
        bg_color = convert_color(173, 216, 230);  // Light blue background
        fg_color = convert_color(0, 0, 0);        // Black foreground
        break;
    case PIXEL_FORMAT_BGR_565:
        fill_buffer_fnc = fill_buffer_bgr565;
        frame_buffer_size *= 2;
        bg_color = convert_color(173, 216, 230);  // Light blue background
        fg_color = convert_color(0, 0, 0);        // Black foreground
        break;
    case PIXEL_FORMAT_MONO01:
        fill_buffer_fnc = fill_buffer_mono01;
        frame_buffer_size = DIV_ROUND_UP(frame_buffer_size, 8);
        bg_color = 0;
        fg_color = 1;
        break;
    case PIXEL_FORMAT_MONO10:
        fill_buffer_fnc = fill_buffer_mono10;
        frame_buffer_size = DIV_ROUND_UP(frame_buffer_size, 8);
        bg_color = 0;
        fg_color = 1;
        break;
    default:
        LOG_ERR("Unsupported pixel format: %d", capabilities.current_pixel_format);
        return -ENOTSUP;
    }

    // Check if frame buffer is large enough
    if (frame_buffer_size > sizeof(frame_buffer)) {
        LOG_ERR("Frame buffer too large (%zu bytes), max is %zu", 
                frame_buffer_size, sizeof(frame_buffer));
        return -ENOMEM;
    }

    // Setup buffer descriptor
    buf_desc.buf_size = frame_buffer_size;
    buf_desc.width = capabilities.x_resolution;
    buf_desc.height = capabilities.y_resolution;
    buf_desc.pitch = capabilities.x_resolution;
    buf_desc.frame_incomplete = false;

    // Clear display and turn it on
    display_engine_clear(0x000000); // Clear with black background
    display_blanking_off(display_dev);

    LOG_INF("Display engine initialized successfully");
    return 0;
}

void display_engine_clear(uint32_t color)
{
    if (!display_dev || !fill_buffer_fnc) {
        return;
    }
    
    fill_buffer_fnc(color, frame_buffer, frame_buffer_size);
    bg_color = color;
}

// Helper function to draw a single character
static void draw_char(char ch, int x, int y, uint32_t color)
{
    const uint8_t *char_data = font_get_char_data(ch);
    if (!char_data) {
        return;
    }
    
    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t row_data = char_data[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            if (row_data & (0x10 >> col)) {  // Check bits 4,3,2,1,0 for 5-bit font
                display_engine_set_pixel(x + col, y + row, color);
            }
        }
    }
}

// Helper function to draw a large character (2x scale)
static void draw_char_large(char ch, int x, int y, uint32_t color)
{
    const uint8_t *char_data = font_get_char_data(ch);
    if (!char_data) {
        return;
    }
    
    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t row_data = char_data[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            if (row_data & (0x10 >> col)) {  // Check bits 4,3,2,1,0 for 5-bit font
                // Draw 2x2 pixel block
                int px = x + (col * 2);
                int py = y + (row * 2);
                display_engine_set_pixel(px, py, color);
                display_engine_set_pixel(px + 1, py, color);
                display_engine_set_pixel(px, py + 1, color);
                display_engine_set_pixel(px + 1, py + 1, color);
            }
        }
    }
}

void display_engine_draw_text(const char *text, int x, int y, uint32_t color)
{
    if (!text || !display_dev) {
        return;
    }
    
    int char_x = x;
    int char_y = y;
    
    for (const char *p = text; *p; p++) {
        if (*p == '\n') {
            char_y += FONT_HEIGHT + 2;
            char_x = x;
            continue;
        }
        
        draw_char(*p, char_x, char_y, color);
        char_x += FONT_WIDTH;
    }
}

void display_engine_draw_text_large(const char *text, int x, int y, uint32_t color)
{
    if (!text || !display_dev) {
        return;
    }
    
    int char_x = x;
    int char_y = y;
    
    for (const char *p = text; *p; p++) {
        if (*p == '\n') {
            char_y += (FONT_HEIGHT * 2) + 4;
            char_x = x;
            continue;
        }
        
        draw_char_large(*p, char_x, char_y, color);
        char_x += FONT_WIDTH * 2;
    }
}

void display_engine_fill_rect(int x, int y, int w, int h, uint32_t color)
{
    if (!display_dev) {
        return;
    }
    
    // Bounds checking
    if (x < 0 || y < 0 || x >= capabilities.x_resolution || y >= capabilities.y_resolution) {
        return;
    }
    
    if (x + w > capabilities.x_resolution) {
        w = capabilities.x_resolution - x;
    }
    if (y + h > capabilities.y_resolution) {
        h = capabilities.y_resolution - y;
    }
    
    // Draw filled rectangle
    for (int py = y; py < y + h; py++) {
        switch (capabilities.current_pixel_format) {
        case PIXEL_FORMAT_ARGB_8888: {
            uint32_t *line = (uint32_t*)(frame_buffer + 
                py * capabilities.x_resolution * 4 + x * 4);
            for (int col = 0; col < w; col++) {
                line[col] = color;
            }
            break;
        }
        case PIXEL_FORMAT_RGB_888: {
            uint8_t *line = frame_buffer + 
                py * capabilities.x_resolution * 3 + x * 3;
            for (int col = 0; col < w; col++) {
                line[col * 3 + 0] = (color >> 16) & 0xFF;
                line[col * 3 + 1] = (color >> 8) & 0xFF;
                line[col * 3 + 2] = color & 0xFF;
            }
            break;
        }
        case PIXEL_FORMAT_RGB_565:
        case PIXEL_FORMAT_BGR_565: {
            uint16_t *line = (uint16_t*)(frame_buffer + 
                py * capabilities.x_resolution * 2 + x * 2);
            for (int col = 0; col < w; col++) {
                line[col] = (uint16_t)color;
            }
            break;
        }
        default:
            // Monochrome formats - simplified implementation
            break;
        }
    }
}

void display_engine_set_pixel(int x, int y, uint32_t color)
{
    if (!display_dev || x < 0 || y < 0 || 
        x >= capabilities.x_resolution || y >= capabilities.y_resolution) {
        return;
    }
    
    switch (capabilities.current_pixel_format) {
    case PIXEL_FORMAT_ARGB_8888: {
        uint32_t *pixel = (uint32_t*)(frame_buffer + 
            y * capabilities.x_resolution * 4 + x * 4);
        *pixel = color;
        break;
    }
    case PIXEL_FORMAT_RGB_888: {
        uint8_t *pixel = frame_buffer + 
            y * capabilities.x_resolution * 3 + x * 3;
        pixel[0] = (color >> 16) & 0xFF;
        pixel[1] = (color >> 8) & 0xFF;
        pixel[2] = color & 0xFF;
        break;
    }
    case PIXEL_FORMAT_RGB_565:
    case PIXEL_FORMAT_BGR_565: {
        uint16_t *pixel = (uint16_t*)(frame_buffer + 
            y * capabilities.x_resolution * 2 + x * 2);
        *pixel = (uint16_t)color;
        break;
    }
    default:
        // Monochrome formats - simplified implementation
        break;
    }
}

void display_engine_present(void)
{
    if (!display_dev || frame_buffer_size == 0) {
        return;
    }
    
    display_write(display_dev, 0, 0, &buf_desc, frame_buffer);
}

void display_engine_update(void)
{
    // Update/present the display buffer to the screen
    display_engine_present();
}

int display_engine_get_width(void)
{
    return capabilities.x_resolution;
}

int display_engine_get_height(void)
{
    return capabilities.y_resolution;
}

void display_engine_set_blanking(bool on)
{
    if (!display_dev) {
        return;
    }
    
    if (on) {
        display_blanking_on(display_dev);
    } else {
        display_blanking_off(display_dev);
    }
}
