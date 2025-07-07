/*
 * Calculator Core - State machine and calculation logic
 */

#ifndef CALCULATOR_CORE_H
#define CALCULATOR_CORE_H

#include "keypad_handler.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Calculator states
 */
typedef enum {
    STATE_INPUT_NORMAL,  // Normal input expression mode
    STATE_SHOW_RESULT,   // Displaying calculation result
    STATE_SHOW_ERROR,    // Displaying error message
    STATE_MENU_MODE,     // Menu/function selection
    STATE_MATRIX_MODE,   // Matrix operations
    STATE_VECTOR_MODE,   // Vector operations
    STATE_SOLVE_MODE,    // Equation solving
    STATE_SETUP_MODE,    // Settings/configuration
} calculator_state_t;

/**
 * @brief Calculator mode flags
 */
typedef struct {
    bool shift_mode;     // SHIFT key pressed
    bool alpha_mode;     // ALPHA key pressed  
    bool deg_mode;       // Degree mode (vs radians)
    bool complex_mode;   // Complex number mode
    bool stat_mode;      // Statistics mode
} calculator_mode_t;

/**
 * @brief Calculator data structure
 */
struct calculator {
    calculator_state_t state;
    calculator_mode_t mode;
    char input_buffer[128];     // User input expression (increased size)
    int input_pos;              // Current position in input buffer
    char result_buffer[64];     // Calculation result display
    char error_buffer[64];      // Error message display
    char status_buffer[32];     // Status line (COMP, STAT, etc.)
    double last_result;         // Last calculated result (Ans)
    double memory_x;            // Memory X variable
    double memory_y;            // Memory Y variable
    bool new_number;            // Flag for new number input
    int cursor_pos;             // Cursor position for editing
};

/**
 * @brief Initialize calculator
 * @param calc Calculator instance to initialize
 */
void calculator_init(struct calculator *calc);

/**
 * @brief Update calculator state based on key input
 * @param calc Calculator instance
 * @param key Key code from keypad
 */
void calculator_update_state(struct calculator *calc, key_code_t key);

/**
 * @brief Render calculator UI
 * @param calc Calculator instance
 */
void calculator_render_ui(struct calculator *calc);

/**
 * @brief Evaluate mathematical expression
 * @param expression Null-terminated expression string
 * @param result Pointer to store result
 * @return 0 on success, negative error code on failure
 */
int calculator_evaluate(const char *expression, double *result);

#endif /* CALCULATOR_CORE_H */
