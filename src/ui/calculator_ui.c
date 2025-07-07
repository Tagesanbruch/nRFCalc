/*
 * Calculator UI Renderer Implementation
 */

#include "calculator_ui.h"
#include "../display_engine.h"
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>

LOG_MODULE_REGISTER(calculator_ui, LOG_LEVEL_INF);

// Display dimensions and layout
#define DISPLAY_WIDTH   320
#define DISPLAY_HEIGHT  240
#define STATUS_HEIGHT   20
#define MAIN_DISPLAY_Y  STATUS_HEIGHT
#define MAIN_DISPLAY_HEIGHT (DISPLAY_HEIGHT - STATUS_HEIGHT)

// Colors for ARGB 8888 format (32-bit)
#define COLOR_BLACK     0xFF000000  // Alpha=255, RGB=0,0,0 (opaque black)
#define COLOR_WHITE     0xFFFFFFFF  // Alpha=255, RGB=255,255,255 (opaque white)  
#define COLOR_GRAY      0xFF808080  // Alpha=255, RGB=128,128,128 (opaque gray)
#define COLOR_GREEN     0xFF00FF00  // Alpha=255, RGB=0,255,0 (opaque green)

void calculator_render_ui(calculator_t *calc)
{
    static int render_count = 0;
    if ((render_count % 100) == 0) {  // Log every 100th render to avoid spam
        LOG_INF("Rendering UI (count=%d): state=%d, input='%s'", 
                render_count, calc->state, calc->input_buffer);
    }
    render_count++;
    
    // Clear the display
    display_engine_clear(COLOR_BLACK);
    
    // Render status bar at top
    render_status_bar(calc);
    
    // Render main content based on state
    switch (calc->state) {
        case STATE_INPUT_NORMAL:
        case STATE_SHOW_RESULT:
        case STATE_SHOW_ERROR:
            render_main_display(calc);
            break;
            
        case STATE_MENU_MODE:
            render_mode_menu(calc);
            break;
            
        case STATE_MENU_SETUP:
            render_setup_menu(calc);
            break;
            
        default:
            render_main_display(calc);
            break;
    }
    
    // Update the display
    display_engine_update();
}

void render_status_bar(calculator_t *calc)
{
    // Background for status bar
    display_engine_fill_rect(0, 0, DISPLAY_WIDTH, STATUS_HEIGHT, COLOR_GRAY);
    
    // Calculator mode (left side)
    display_engine_draw_text(calc->status_buffer, 2, 2, COLOR_BLACK);
    
    // Mode indicators (right side)
    int x_pos = DISPLAY_WIDTH - 80;
    
    // Angle mode indicator
    if (calc->mode.deg_mode) {
        display_engine_draw_text("D", x_pos, 2, COLOR_BLACK);
    } else {
        display_engine_draw_text("R", x_pos, 2, COLOR_BLACK);
    }
    x_pos += 15;
    
    // SHIFT indicator
    if (calc->mode.shift_mode) {
        display_engine_draw_text("S", x_pos, 2, COLOR_GREEN);
    }
    x_pos += 15;
    
    // ALPHA indicator  
    if (calc->mode.alpha_mode) {
        display_engine_draw_text("A", x_pos, 2, COLOR_GREEN);
    }
    x_pos += 15;
    
    // Display mode indicators
    if (calc->mode.fix_mode) {
        display_engine_draw_text("FIX", x_pos, 2, COLOR_BLACK);
    } else if (calc->mode.sci_mode) {
        display_engine_draw_text("SCI", x_pos, 2, COLOR_BLACK);
    } else if (calc->mode.eng_mode) {
        display_engine_draw_text("ENG", x_pos, 2, COLOR_BLACK);
    }
}

void render_main_display(calculator_t *calc)
{
    int y_pos = MAIN_DISPLAY_Y + 10;
    
    // Render input expression (small font, top line)
    if (calc->state == STATE_INPUT_NORMAL || 
        (calc->state == STATE_SHOW_RESULT && strlen(calc->input_buffer) > 1)) {
        display_engine_draw_text(calc->input_buffer, 10, y_pos, COLOR_WHITE);
        
        // Render cursor if in input mode
        if (calc->state == STATE_INPUT_NORMAL) {
            // Calculate cursor position (approximate)
            int cursor_x = 10 + (calc->cursor_pos * 8); // Assuming 8 pixels per character
            render_cursor(calc, cursor_x, y_pos);
        }
        
        y_pos += 25;
    }
    
    // Render result or error (large font, main line)
    if (calc->state == STATE_SHOW_RESULT) {
        // Right-align the result
        int text_width = strlen(calc->result_buffer) * 12; // Assuming 12 pixels per character for large font
        int x_pos = DISPLAY_WIDTH - text_width - 10;
        display_engine_draw_text_large(calc->result_buffer, x_pos, y_pos + 20, COLOR_WHITE);
    } else if (calc->state == STATE_SHOW_ERROR) {
        // Center the error message
        int text_width = strlen(calc->error_buffer) * 8;
        int x_pos = (DISPLAY_WIDTH - text_width) / 2;
        display_engine_draw_text(calc->error_buffer, x_pos, y_pos + 30, COLOR_WHITE);
    } else {
        // Show current input in large font
        int text_width = strlen(calc->input_buffer) * 12;
        int x_pos = DISPLAY_WIDTH - text_width - 10;
        display_engine_draw_text_large(calc->input_buffer, x_pos, y_pos + 20, COLOR_WHITE);
    }
}

void render_mode_menu(calculator_t *calc)
{
    int y_pos = MAIN_DISPLAY_Y + 20;
    
    // Menu title
    display_engine_draw_text("MODE", 10, y_pos, COLOR_WHITE);
    y_pos += 25;
    
    // Menu options
    const char* mode_options[] = {
        "1: COMP    (Computation)",
        "2: STAT    (Statistics)", 
        "3: BASE-N  (Base-n)",
        "4: EQN     (Equation)",
        "5: MATRIX  (Matrix)",
        "6: VECTOR  (Vector)",
        "7: CMPLX   (Complex)",
        "8: TABLE   (Table)"
    };
    
    for (int i = 0; i < 8; i++) {
        if (i == calc->menu_selection) {
            // Highlight selected option
            display_engine_fill_rect(5, y_pos - 2, DISPLAY_WIDTH - 10, 16, COLOR_GRAY);
            display_engine_draw_text(mode_options[i], 10, y_pos, COLOR_BLACK);
        } else {
            display_engine_draw_text(mode_options[i], 10, y_pos, COLOR_WHITE);
        }
        y_pos += 18;
    }
    
    // Help text
    y_pos += 10;
    display_engine_draw_text("AC: Exit", 10, y_pos, COLOR_GRAY);
}

void render_setup_menu(calculator_t *calc)
{
    int y_pos = MAIN_DISPLAY_Y + 20;
    
    // Menu title
    display_engine_draw_text("SETUP", 10, y_pos, COLOR_WHITE);
    y_pos += 25;
    
    // Setup options
    const char* setup_options[] = {
        "1: Angle Unit",
        "2: Display Format", 
        "3: Number Format",
        "4: Stat Freq",
        "5: Reset"
    };
    
    for (int i = 0; i < 5; i++) {
        if (i == calc->setup_selection) {
            // Highlight selected option
            display_engine_fill_rect(5, y_pos - 2, DISPLAY_WIDTH - 10, 16, COLOR_GRAY);
            display_engine_draw_text(setup_options[i], 10, y_pos, COLOR_BLACK);
        } else {
            display_engine_draw_text(setup_options[i], 10, y_pos, COLOR_WHITE);
        }
        y_pos += 18;
    }
    
    // Current settings display
    y_pos += 15;
    char settings_text[64];
    snprintf(settings_text, sizeof(settings_text), 
             "Angle: %s  Format: %s", 
             calc->mode.deg_mode ? "Deg" : "Rad",
             calc->mode.fix_mode ? "Fix" : calc->mode.sci_mode ? "Sci" : "Norm");
    display_engine_draw_text(settings_text, 10, y_pos, COLOR_GRAY);
    
    // Help text
    y_pos += 15;
    display_engine_draw_text("AC: Exit", 10, y_pos, COLOR_GRAY);
}

void render_cursor(calculator_t *calc, int x, int y)
{
    static bool cursor_visible = true;
    static int cursor_timer = 0;
    
    // Blink cursor every 30 frames (approximately 1 second at 30 FPS)
    cursor_timer++;
    if (cursor_timer >= 30) {
        cursor_visible = !cursor_visible;
        cursor_timer = 0;
    }
    
    if (cursor_visible) {
        // Draw cursor as a vertical line
        for (int i = 0; i < 12; i++) {
            display_engine_set_pixel(x, y + i, COLOR_WHITE);
        }
    }
}
