/*
 * Calculator State Machine Implementation
 */

#include "calculator_state.h"
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

LOG_MODULE_REGISTER(calculator_state, LOG_LEVEL_INF);

// State name strings for debugging
static const char* state_names[] = {
    "INPUT_NORMAL", "SHOW_RESULT", "SHOW_ERROR", "MENU_MODE", "MENU_SETUP",
    "MATRIX_MODE", "VECTOR_MODE", "SOLVE_MODE", "STAT_MODE", "BASE_N_MODE",
    "COMPLEX_MODE", "TABLE_MODE", "EQUATION_MODE", "INTEGRAL_MODE", "DIFFERENTIAL_MODE"
};

const char* get_state_name(calculator_state_t state)
{
    if (state < sizeof(state_names) / sizeof(state_names[0])) {
        return state_names[state];
    }
    return "UNKNOWN";
}

void calculator_init(calculator_t *calc)
{
    memset(calc, 0, sizeof(*calc));
    
    // Set initial state
    calc->state = STATE_INPUT_NORMAL;
    calc->prev_state = STATE_INPUT_NORMAL;
    
    // Set default modes
    calc->mode.deg_mode = true;  // Default to degree mode
    calc->mode.decimal_places = 2;
    
    // Initialize buffers
    strcpy(calc->input_buffer, "0");
    strcpy(calc->status_buffer, "COMP");
    calc->input_pos = 1;
    calc->cursor_pos = 1;
    calc->new_number = true;
    
    // Initialize evaluation context
    calc->eval_context.deg_mode = calc->mode.deg_mode;
    memset(&calc->eval_context.variables, 0, sizeof(calc->eval_context.variables));
    
    LOG_INF("Calculator initialized in %s state", get_state_name(calc->state));
}

void calculator_clear(calculator_t *calc)
{
    memset(calc->input_buffer, 0, sizeof(calc->input_buffer));
    strcpy(calc->input_buffer, "0");
    calc->input_pos = 1;
    calc->cursor_pos = 1;
    calc->new_number = true;
    calc->calculation_done = false;
    calc->error_state = false;
    calc->state = STATE_INPUT_NORMAL;
    memset(calc->result_buffer, 0, sizeof(calc->result_buffer));
    memset(calc->error_buffer, 0, sizeof(calc->error_buffer));
}

void calculator_clear_memory(calculator_t *calc)
{
    memset(&calc->memory, 0, sizeof(calc->memory));
    memset(&calc->eval_context.variables, 0, sizeof(calc->eval_context.variables));
    LOG_INF("All memory cleared");
}

void calculator_set_error(calculator_t *calc, const char *error_msg)
{
    strncpy(calc->error_buffer, error_msg, sizeof(calc->error_buffer) - 1);
    calc->error_buffer[sizeof(calc->error_buffer) - 1] = '\0';
    calc->state = STATE_SHOW_ERROR;
    calc->error_state = true;
    LOG_WRN("Calculator error: %s", error_msg);
}

static void append_char(calculator_t *calc, char ch)
{
    if (calc->new_number) {
        // Starting a new number, clear the display
        calculator_clear(calc);
        calc->new_number = false;
    }
    
    // Remove leading zero if we're entering the first digit
    if (calc->input_pos == 1 && calc->input_buffer[0] == '0' && ch != '.') {
        calc->input_buffer[0] = ch;
        calc->cursor_pos = 1;
        return;
    }
    
    if (calc->input_pos < sizeof(calc->input_buffer) - 1) {
        calc->input_buffer[calc->input_pos++] = ch;
        calc->input_buffer[calc->input_pos] = '\0';
        calc->cursor_pos = calc->input_pos;
    }
}

static void append_string(calculator_t *calc, const char *str)
{
    if (calc->new_number && strcmp(str, "Ans") != 0) {
        calculator_clear(calc);
        calc->new_number = false;
    }
    
    int len = strlen(str);
    if (calc->input_pos + len < sizeof(calc->input_buffer) - 1) {
        strcpy(&calc->input_buffer[calc->input_pos], str);
        calc->input_pos += len;
        calc->cursor_pos = calc->input_pos;
    }
}

static void append_operator(calculator_t *calc, char op)
{
    // If we're showing a result, use it as the start of new expression
    if (calc->state == STATE_SHOW_RESULT) {
        snprintf(calc->input_buffer, sizeof(calc->input_buffer), 
                 "%.10g%c", calc->memory.ans, op);
        calc->input_pos = strlen(calc->input_buffer);
        calc->cursor_pos = calc->input_pos;
        calc->state = STATE_INPUT_NORMAL;
        calc->new_number = false;
        return;
    }
    
    // Don't allow operators at the beginning (except minus)
    if (calc->input_pos == 0 && op != '-') {
        return;
    }
    
    // Replace last character if it's already an operator
    if (calc->input_pos > 0) {
        char last_char = calc->input_buffer[calc->input_pos - 1];
        if (last_char == '+' || last_char == '-' || last_char == '*' || last_char == '/') {
            calc->input_buffer[calc->input_pos - 1] = op;
            calc->cursor_pos = calc->input_pos;
            return;
        }
    }
    
    if (calc->input_pos < sizeof(calc->input_buffer) - 1) {
        calc->input_buffer[calc->input_pos++] = op;
        calc->input_buffer[calc->input_pos] = '\0';
        calc->cursor_pos = calc->input_pos;
    }
    calc->new_number = false;
}

static void backspace(calculator_t *calc)
{
    if (calc->state == STATE_SHOW_RESULT || calc->state == STATE_SHOW_ERROR) {
        // Return to input mode
        calculator_clear(calc);
        return;
    }
    
    if (calc->input_pos > 1) {
        calc->input_buffer[--calc->input_pos] = '\0';
        calc->cursor_pos = calc->input_pos;
    } else if (calc->input_pos == 1) {
        // Replace with 0
        calc->input_buffer[0] = '0';
        calc->input_buffer[1] = '\0';
        calc->cursor_pos = 1;
    }
}

void calculator_execute(calculator_t *calc)
{
    if (strlen(calc->input_buffer) == 0 || strcmp(calc->input_buffer, "0") == 0) {
        return;
    }
    
    // Update evaluation context with current variables
    calc->eval_context.variables = (variable_storage_t){
        .ans = calc->memory.ans,
        .x = calc->memory.x, .y = calc->memory.y,
        .a = calc->memory.a, .b = calc->memory.b,
        .c = calc->memory.c, .d = calc->memory.d,
        .m = calc->memory.m
    };
    calc->eval_context.deg_mode = calc->mode.deg_mode;
    
    double result;
    int eval_result = evaluate_expression(calc->input_buffer, &calc->eval_context, &result);
    
    if (eval_result == 0) {
        // Success
        calc->memory.ans = result;
        calc->memory.has_ans = true;
        
        // Format result based on display mode
        if (calc->mode.sci_mode) {
            snprintf(calc->result_buffer, sizeof(calc->result_buffer), 
                     "%.6e", result);
        } else if (calc->mode.fix_mode) {
            char format[16];
            snprintf(format, sizeof(format), "%%.%df", calc->mode.decimal_places);
            snprintf(calc->result_buffer, sizeof(calc->result_buffer), 
                     format, result);
        } else {
            snprintf(calc->result_buffer, sizeof(calc->result_buffer), 
                     "%.10g", result);
        }
        
        calc->state = STATE_SHOW_RESULT;
        calc->calculation_done = true;
        calc->new_number = true;
        
        LOG_INF("Calculation: %s = %g", calc->input_buffer, result);
    } else {
        // Error
        const char *error_msg;
        switch (eval_result) {
            case -1: error_msg = "Syntax Error"; break;
            case -2: error_msg = "Math Error"; break;
            case -3: error_msg = "Domain Error"; break;
            case -4: error_msg = "Overflow"; break;
            default: error_msg = "Error"; break;
        }
        calculator_set_error(calc, error_msg);
    }
}

// Handle normal input state
static void handle_normal_input(calculator_t *calc, key_code_t key)
{
    switch (key) {
        // Numbers
        case KEY_0: case KEY_1: case KEY_2: case KEY_3: case KEY_4:
        case KEY_5: case KEY_6: case KEY_7: case KEY_8: case KEY_9:
            append_char(calc, '0' + (key - KEY_0));
            break;
            
        case KEY_DOT:
            // Don't allow multiple decimal points
            if (strchr(calc->input_buffer, '.') == NULL) {
                append_char(calc, '.');
            }
            break;
            
        // Basic operators
        case KEY_PLUS:
            append_operator(calc, '+');
            break;
        case KEY_MINUS:
            append_operator(calc, '-');
            break;
        case KEY_MULTIPLY:
            append_operator(calc, '*');
            break;
        case KEY_DIVIDE:
            append_operator(calc, '/');
            break;
            
        // Functions (add opening parenthesis)
        case KEY_SIN:
            if (calc->mode.shift_mode) {
                append_string(calc, "asin(");
            } else {
                append_string(calc, "sin(");
            }
            break;
        case KEY_COS:
            if (calc->mode.shift_mode) {
                append_string(calc, "acos(");
            } else {
                append_string(calc, "cos(");
            }
            break;
        case KEY_TAN:
            if (calc->mode.shift_mode) {
                append_string(calc, "atan(");
            } else {
                append_string(calc, "tan(");
            }
            break;
        case KEY_LOG:
            if (calc->mode.shift_mode) {
                append_string(calc, "exp("); // 10^x is exp for base 10
            } else {
                append_string(calc, "log(");
            }
            break;
        case KEY_LN:
            if (calc->mode.shift_mode) {
                append_string(calc, "exp(");
            } else {
                append_string(calc, "ln(");
            }
            break;
        case KEY_SQRT:
            if (calc->mode.shift_mode) {
                append_char(calc, '^');
                append_char(calc, '2');
            } else {
                append_string(calc, "sqrt(");
            }
            break;
            
        // Constants
        case KEY_EXP:
            if (calc->mode.shift_mode) {
                append_string(calc, "π");
            } else {
                append_string(calc, "*10^");
            }
            break;
            
        // Parentheses
        case KEY_PAREN_LEFT:
            append_char(calc, '(');
            break;
        case KEY_PAREN_RIGHT:
            append_char(calc, ')');
            break;
            
        // Variables
        case KEY_ANS:
            append_string(calc, "Ans");
            break;
            
        // Power
        case KEY_X_POW_Y:
            append_char(calc, '^');
            break;
            
        // Execute calculation
        case KEY_EQUAL:
            calculator_execute(calc);
            break;
            
        // Clear and backspace
        case KEY_CLEAR:
        case KEY_ON_AC:
            calculator_clear(calc);
            break;
        case KEY_BACKSPACE:
            backspace(calc);
            break;
            
        default:
            // Ignore unknown keys
            break;
    }
}

void calculator_update_state(calculator_t *calc, key_code_t key)
{
    if (key != KEY_NONE) {
        LOG_INF("Updating state: current=%s, key=%d", get_state_name(calc->state), key);
    }
    LOG_DBG("State: %s, Key: %d", get_state_name(calc->state), key);
    
    // Handle mode keys first (they work in all states)
    if (key == KEY_SHIFT) {
        calc->mode.shift_mode = !calc->mode.shift_mode;
        LOG_INF("SHIFT mode: %s", calc->mode.shift_mode ? "ON" : "OFF");
        return;
    }
    
    if (key == KEY_ALPHA) {
        calc->mode.alpha_mode = !calc->mode.alpha_mode;
        LOG_INF("ALPHA mode: %s", calc->mode.alpha_mode ? "ON" : "OFF");
        return;
    }
    
    if (key == KEY_MODE) {
        calc->prev_state = calc->state;
        calc->state = STATE_MENU_MODE;
        calc->menu_selection = 0;
        return;
    }
    
    // Handle state-specific keys
    switch (calc->state) {
        case STATE_INPUT_NORMAL:
            handle_normal_input(calc, key);
            break;
            
        case STATE_SHOW_RESULT:
            // In result mode, only specific keys should transition back to input
            if (key == KEY_CLEAR || key == KEY_ON_AC) {
                // Clear key always starts fresh
                calculator_clear(calc);
                calc->state = STATE_INPUT_NORMAL;
            } else if (key >= KEY_0 && key <= KEY_9 || key == KEY_DOT) {
                // Number keys start a new calculation
                calculator_clear(calc);
                calc->state = STATE_INPUT_NORMAL;
                handle_normal_input(calc, key);
            } else if (key == KEY_PLUS || key == KEY_MINUS || key == KEY_MULTIPLY || key == KEY_DIVIDE) {
                // Operator keys continue with the result
                snprintf(calc->input_buffer, sizeof(calc->input_buffer), 
                         "%.10g", calc->memory.ans);
                calc->input_pos = strlen(calc->input_buffer);
                calc->cursor_pos = calc->input_pos;
                calc->state = STATE_INPUT_NORMAL;
                calc->new_number = false;
                handle_normal_input(calc, key);
            } else if (key == KEY_EQUAL) {
                // Equal key does nothing in result mode (stay in result)
                return;
            } else if (key != KEY_SHIFT && key != KEY_ALPHA && key != KEY_MODE && key != KEY_NONE) {
                // Other function keys start fresh calculation
                calculator_clear(calc);
                calc->state = STATE_INPUT_NORMAL;
                handle_normal_input(calc, key);
            }
            break;
            
        case STATE_SHOW_ERROR:
            // In error mode, any key (except mode keys) clears the error
            if (key != KEY_SHIFT && key != KEY_ALPHA && key != KEY_MODE && key != KEY_NONE) {
                calculator_clear(calc);
                calc->state = STATE_INPUT_NORMAL;
                // Only handle the key if it's a valid input key
                if ((key >= KEY_0 && key <= KEY_9) || key == KEY_DOT || 
                    key == KEY_PLUS || key == KEY_MINUS || key == KEY_MULTIPLY || key == KEY_DIVIDE ||
                    key == KEY_PAREN_LEFT || key == KEY_PAREN_RIGHT ||
                    key == KEY_SIN || key == KEY_COS || key == KEY_TAN || key == KEY_LOG || 
                    key == KEY_LN || key == KEY_SQRT) {
                    handle_normal_input(calc, key);
                }
            }
            break;
            
        case STATE_MENU_MODE:
            // Handle menu navigation
            // TODO: Implement menu selection logic
            if (key == KEY_CLEAR || key == KEY_ON_AC) {
                calc->state = calc->prev_state;
            }
            break;
            
        default:
            LOG_WRN("Unhandled state: %s", get_state_name(calc->state));
            break;
    }
    
    // Clear mode flags after processing (except for SHIFT/ALPHA keys)
    if (key != KEY_SHIFT && key != KEY_ALPHA && key != KEY_MODE) {
        if (calc->mode.shift_mode || calc->mode.alpha_mode) {
            calc->mode.shift_mode = false;
            calc->mode.alpha_mode = false;
            LOG_DBG("Mode flags cleared");
        }
    }
}

void calculator_select_mode(calculator_t *calc, int selection)
{
    // TODO: Implement mode selection
    calc->state = calc->prev_state;
}

void calculator_select_setup(calculator_t *calc, int selection)
{
    // TODO: Implement setup selection
    calc->state = calc->prev_state;
}
