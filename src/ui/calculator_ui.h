/*
 * Calculator UI Renderer
 * 
 * This module handles all calculator UI rendering, including:
 * - Main calculator display (input, result, error)
 * - Status bar with mode indicators
 * - Menu systems
 * - Cursor and text editing visualization
 */

#ifndef CALCULATOR_UI_H
#define CALCULATOR_UI_H

#include "../state/calculator_state.h"
#include "../display_engine.h"

/**
 * @brief Render the complete calculator UI
 * @param calc Calculator instance
 */
void calculator_render_ui(calculator_t *calc);

/**
 * @brief Render the main display area
 * @param calc Calculator instance
 */
void render_main_display(calculator_t *calc);

/**
 * @brief Render the status bar
 * @param calc Calculator instance
 */
void render_status_bar(calculator_t *calc);

/**
 * @brief Render mode selection menu
 * @param calc Calculator instance
 */
void render_mode_menu(calculator_t *calc);

/**
 * @brief Render setup menu
 * @param calc Calculator instance
 */
void render_setup_menu(calculator_t *calc);

/**
 * @brief Render cursor at current position
 * @param calc Calculator instance
 * @param x X position
 * @param y Y position
 */
void render_cursor(calculator_t *calc, int x, int y);

#endif /* CALCULATOR_UI_H */
