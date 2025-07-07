/*
 * Calculator State Machine
 * 
 * This module manages the calculator's state transitions and UI modes,
 * implementing the state machine for different calculator modes like
 * COMP, STAT, MATRIX, VECTOR, etc.
 */

#ifndef CALCULATOR_STATE_H
#define CALCULATOR_STATE_H

#include "../keypad_handler.h"
#include "../math/expression_evaluator.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Calculator states
 */
typedef enum {
    STATE_INPUT_NORMAL,     // Normal input expression mode
    STATE_SHOW_RESULT,      // Displaying calculation result
    STATE_SHOW_ERROR,       // Displaying error message
    STATE_MENU_MODE,        // Mode selection menu
    STATE_MENU_SETUP,       // Setup/configuration menu
    STATE_MATRIX_MODE,      // Matrix operations
    STATE_VECTOR_MODE,      // Vector operations
    STATE_SOLVE_MODE,       // Equation solving mode
    STATE_STAT_MODE,        // Statistics mode
    STATE_BASE_N_MODE,      // Base-N calculations
    STATE_COMPLEX_MODE,     // Complex number mode
    STATE_TABLE_MODE,       // Table calculation mode
    STATE_EQUATION_MODE,    // Equation mode
    STATE_INTEGRAL_MODE,    // Integration mode
    STATE_DIFFERENTIAL_MODE // Differentiation mode
} calculator_state_t;

/**
 * @brief Calculator mode flags
 */
typedef struct {
    bool shift_mode;        // SHIFT key active
    bool alpha_mode;        // ALPHA key active  
    bool deg_mode;          // Degree mode (vs radians)
    bool complex_mode;      // Complex number mode
    bool stat_mode;         // Statistics mode
    bool fix_mode;          // Fixed decimal places
    bool sci_mode;          // Scientific notation
    bool eng_mode;          // Engineering notation
    int decimal_places;     // Number of decimal places (for FIX mode)
} calculator_mode_t;

/**
 * @brief Memory storage structure
 */
typedef struct {
    double ans;             // Last result (Ans)
    double x, y;            // Variables X, Y
    double a, b, c, d;      // Memory variables A-D
    double m;               // Memory M
    bool has_ans;           // True if Ans has been set
} memory_storage_t;

/**
 * @brief Main calculator data structure
 */
typedef struct {
    calculator_state_t state;
    calculator_state_t prev_state;  // For returning from menus
    calculator_mode_t mode;
    
    // Input/output buffers
    char input_buffer[128];         // User input expression
    int input_pos;                  // Current position in input buffer
    int cursor_pos;                 // Cursor position for editing
    char result_buffer[64];         // Calculation result display
    char error_buffer[64];          // Error message display
    char status_buffer[32];         // Status line (COMP, STAT, etc.)
    
    // Memory and variables
    memory_storage_t memory;
    
    // State flags
    bool new_number;                // Flag for new number input
    bool calculation_done;          // Flag for completed calculation
    bool error_state;               // Flag for error condition
    
    // Menu state
    int menu_selection;             // Current menu selection
    int setup_selection;            // Current setup selection
    
    // Evaluation context
    eval_context_t eval_context;
} calculator_t;

/**
 * @brief Initialize calculator
 * @param calc Calculator instance to initialize
 */
void calculator_init(calculator_t *calc);

/**
 * @brief Update calculator state based on key input
 * @param calc Calculator instance
 * @param key Key code from keypad
 */
void calculator_update_state(calculator_t *calc, key_code_t key);

/**
 * @brief Get current state name for debugging
 * @param state Calculator state
 * @return State name string
 */
const char* get_state_name(calculator_state_t state);

/**
 * @brief Clear calculator input and reset to normal state
 * @param calc Calculator instance
 */
void calculator_clear(calculator_t *calc);

/**
 * @brief Clear all memory variables
 * @param calc Calculator instance
 */
void calculator_clear_memory(calculator_t *calc);

/**
 * @brief Set calculator to error state
 * @param calc Calculator instance
 * @param error_msg Error message to display
 */
void calculator_set_error(calculator_t *calc, const char *error_msg);

/**
 * @brief Execute calculation and update state
 * @param calc Calculator instance
 */
void calculator_execute(calculator_t *calc);

/**
 * @brief Handle mode selection
 * @param calc Calculator instance
 * @param selection Mode selection index
 */
void calculator_select_mode(calculator_t *calc, int selection);

/**
 * @brief Handle setup selection
 * @param calc Calculator instance
 * @param selection Setup selection index
 */
void calculator_select_setup(calculator_t *calc, int selection);

#endif /* CALCULATOR_STATE_H */
