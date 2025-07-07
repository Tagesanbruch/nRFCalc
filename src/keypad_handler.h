/*
 * Keypad Handler - Unified input abstraction layer
 *
 * This module provides a unified interface for getting key inputs,
 * supporting both SDL simulation and hardware GPIO inputs.
 */

#ifndef KEYPAD_HANDLER_H
#define KEYPAD_HANDLER_H

/**
 * @brief Key codes for calculator operations
 */
typedef enum {
    KEY_NONE = 0,
    
    // Numbers
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4,
    KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
    
    // Basic operations
    KEY_PLUS,      // +
    KEY_MINUS,     // -
    KEY_MULTIPLY,  // *
    KEY_DIVIDE,    // /
    KEY_EQUAL,     // =
    KEY_CLEAR,     // C/AC
    KEY_DOT,       // .
    KEY_BACKSPACE, // Del
    
    // Scientific functions
    KEY_SIN, KEY_COS, KEY_TAN,
    KEY_LOG, KEY_LN, KEY_SQRT,
    KEY_POWER, KEY_FACTORIAL, KEY_PI,
    KEY_E, KEY_PAREN_LEFT, KEY_PAREN_RIGHT,
    
    // Casio fx-991 specific keys
    KEY_SHIFT, KEY_ALPHA, KEY_MODE,
    KEY_ON_AC, KEY_X_POW_Y, KEY_X_POW_MINUS1,
    KEY_LOG10, KEY_EXP, KEY_PERCENT,
    KEY_ANS, KEY_ENG, KEY_SETUP,
    KEY_STAT, KEY_MATRIX, KEY_VECTOR,
    KEY_CMPLX, KEY_BASE_N, KEY_EQUATION,
    KEY_CALC, KEY_SOLVE, KEY_INTEGRATE,
    KEY_DIFF, KEY_TABLE, KEY_RESET,
    KEY_RAN_HASH, KEY_DRG, KEY_HYP,
    KEY_STO, KEY_RCL, KEY_CONST,
    KEY_CONV, KEY_FUNC, KEY_OPTN,
    
    KEY_MAX
} key_code_t;

/**
 * @brief Initialize the keypad handler
 * @return 0 on success, negative error code on failure
 */
int keypad_init(void);

/**
 * @brief Get the next key press (non-blocking)
 * @return Key code, or KEY_NONE if no key pressed
 */
key_code_t keypad_get_key(void);

/**
 * @brief Wait for a key press (blocking)
 * @param timeout_ms Maximum time to wait in milliseconds (0 = wait forever)
 * @return Key code, or KEY_NONE if timeout
 */
key_code_t keypad_wait_key(int timeout_ms);

#endif /* KEYPAD_HANDLER_H */
