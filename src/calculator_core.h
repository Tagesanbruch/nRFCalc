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
    STATE_MENU_MODE,     // Menu/function selection (future)
} calculator_state_t;

/**
 * @brief Calculator data structure
 */
struct calculator {
    calculator_state_t state;
    char input_buffer[64];      // User input expression
    int input_pos;              // Current position in input buffer
    char result_buffer[64];     // Calculation result display
    char error_buffer[64];      // Error message display
    double last_result;         // Last calculated result
    bool new_number;            // Flag for new number input
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
