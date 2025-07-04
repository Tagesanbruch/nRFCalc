/*
 * Calculator Core Implementation
 * 
 * This module implements the calculator state machine, input handling,
 * and basic mathematical expression evaluation.
 */

#include "calculator_core.h"
#include "display_engine.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

LOG_MODULE_REGISTER(calculator_core, LOG_LEVEL_INF);

void calculator_init(struct calculator *calc)
{
    memset(calc, 0, sizeof(*calc));
    calc->state = STATE_INPUT_NORMAL;
    strcpy(calc->input_buffer, "0");
    calc->input_pos = 1;
    calc->new_number = true;
}

static void clear_input(struct calculator *calc)
{
    memset(calc->input_buffer, 0, sizeof(calc->input_buffer));
    strcpy(calc->input_buffer, "0");
    calc->input_pos = 1;
    calc->new_number = true;
}

static void append_char(struct calculator *calc, char ch)
{
    if (calc->new_number) {
        // Starting a new number, clear the display
        clear_input(calc);
        calc->new_number = false;
    }
    
    // Remove leading zero if we're entering the first digit
    if (calc->input_pos == 1 && calc->input_buffer[0] == '0' && ch != '.') {
        calc->input_buffer[0] = ch;
        return;
    }
    
    if (calc->input_pos < sizeof(calc->input_buffer) - 1) {
        calc->input_buffer[calc->input_pos++] = ch;
        calc->input_buffer[calc->input_pos] = '\0';
    }
}

static void append_operator(struct calculator *calc, char op)
{
    // If we're showing a result, use it as the start of new expression
    if (calc->state == STATE_SHOW_RESULT) {
        snprintf(calc->input_buffer, sizeof(calc->input_buffer), 
                 "%.10g%c", calc->last_result, op);
        calc->input_pos = strlen(calc->input_buffer);
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
            return;
        }
    }
    
    if (calc->input_pos < sizeof(calc->input_buffer) - 1) {
        calc->input_buffer[calc->input_pos++] = op;
        calc->input_buffer[calc->input_pos] = '\0';
    }
    calc->new_number = false;
}

static void backspace(struct calculator *calc)
{
    if (calc->state == STATE_SHOW_RESULT || calc->state == STATE_SHOW_ERROR) {
        // Return to input mode
        clear_input(calc);
        calc->state = STATE_INPUT_NORMAL;
        return;
    }
    
    if (calc->input_pos > 1) {
        calc->input_buffer[--calc->input_pos] = '\0';
    } else {
        clear_input(calc);
    }
}

void calculator_update_state(struct calculator *calc, key_code_t key)
{
    if (!calc || key == KEY_NONE) {
        return;
    }
    
    switch (key) {
    case KEY_0: case KEY_1: case KEY_2: case KEY_3: case KEY_4:
    case KEY_5: case KEY_6: case KEY_7: case KEY_8: case KEY_9:
        if (calc->state != STATE_INPUT_NORMAL) {
            calc->state = STATE_INPUT_NORMAL;
            clear_input(calc);
        }
        append_char(calc, '0' + (key - KEY_0));
        break;
        
    case KEY_DOT:
        if (calc->state != STATE_INPUT_NORMAL) {
            calc->state = STATE_INPUT_NORMAL;
            clear_input(calc);
        }
        // Only add dot if there isn't one already in the current number
        if (!strchr(calc->input_buffer, '.')) {
            append_char(calc, '.');
        }
        break;
        
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
        
    case KEY_EQUAL:
        if (calc->state == STATE_INPUT_NORMAL) {
            double result;
            if (calculator_evaluate(calc->input_buffer, &result) == 0) {
                calc->last_result = result;
                snprintf(calc->result_buffer, sizeof(calc->result_buffer), 
                         "%.10g", result);
                calc->state = STATE_SHOW_RESULT;
                calc->new_number = true;
            } else {
                strcpy(calc->error_buffer, "Error");
                calc->state = STATE_SHOW_ERROR;
            }
        }
        break;
        
    case KEY_CLEAR:
        clear_input(calc);
        calc->state = STATE_INPUT_NORMAL;
        memset(calc->result_buffer, 0, sizeof(calc->result_buffer));
        memset(calc->error_buffer, 0, sizeof(calc->error_buffer));
        calc->last_result = 0;
        break;
        
    case KEY_BACKSPACE:
        backspace(calc);
        break;
        
    default:
        // Ignore other keys for now
        break;
    }
}

void calculator_render_ui(struct calculator *calc)
{
    if (!calc) {
        return;
    }
    
    // Clear display
    display_engine_clear();
    
    // Get display dimensions
    int width = display_engine_get_width();
    int height = display_engine_get_height();
    
    // Draw title bar
    display_engine_draw_rect(0, 0, width, 20, 0x333333);
    display_engine_draw_text(5, 5, "Scientific Calculator");
    
    // Draw main display area background
    display_engine_draw_rect(5, 25, width - 10, 60, 0x000000);
    
    // Display content based on current state
    const char *display_text = "";
    int text_y = 30;
    
    switch (calc->state) {
    case STATE_INPUT_NORMAL:
        display_text = calc->input_buffer;
        break;
        
    case STATE_SHOW_RESULT:
        display_engine_draw_text(10, text_y, calc->input_buffer);
        display_engine_draw_text(10, text_y + 15, "=");
        display_text = calc->result_buffer;
        text_y += 30;
        break;
        
    case STATE_SHOW_ERROR:
        display_engine_draw_text(10, text_y, calc->input_buffer);
        display_text = calc->error_buffer;
        text_y += 15;
        break;
        
    default:
        display_text = "Ready";
        break;
    }
    
    // Draw main text
    display_engine_draw_text(10, text_y, display_text);
    
    // Draw status information
    char status[32];
    snprintf(status, sizeof(status), "State: %d", calc->state);
    display_engine_draw_text(5, height - 20, status);
    
    // Present all drawing operations
    display_engine_present();
}

// Simple expression evaluator using recursive descent parser
// This is a basic implementation for +, -, *, / operations
// For a more robust solution, consider using shunting-yard algorithm

typedef struct {
    const char *expr;
    int pos;
    int len;
} parser_t;

static double parse_number(parser_t *p);
static double parse_factor(parser_t *p);
static double parse_term(parser_t *p);
static double parse_expression(parser_t *p);

static void skip_whitespace(parser_t *p)
{
    while (p->pos < p->len && (p->expr[p->pos] == ' ' || p->expr[p->pos] == '\t')) {
        p->pos++;
    }
}

static double parse_number(parser_t *p)
{
    skip_whitespace(p);
    
    if (p->pos >= p->len) {
        return 0;
    }
    
    char *endptr;
    double result = strtod(&p->expr[p->pos], &endptr);
    p->pos += (endptr - &p->expr[p->pos]);
    
    return result;
}

static double parse_factor(parser_t *p)
{
    skip_whitespace(p);
    
    if (p->pos >= p->len) {
        return 0;
    }
    
    if (p->expr[p->pos] == '(') {
        p->pos++; // skip '('
        double result = parse_expression(p);
        skip_whitespace(p);
        if (p->pos < p->len && p->expr[p->pos] == ')') {
            p->pos++; // skip ')'
        }
        return result;
    }
    
    if (p->expr[p->pos] == '-') {
        p->pos++; // skip '-'
        return -parse_factor(p);
    }
    
    if (p->expr[p->pos] == '+') {
        p->pos++; // skip '+'
        return parse_factor(p);
    }
    
    return parse_number(p);
}

static double parse_term(parser_t *p)
{
    double result = parse_factor(p);
    
    while (p->pos < p->len) {
        skip_whitespace(p);
        if (p->pos >= p->len) break;
        
        char op = p->expr[p->pos];
        if (op != '*' && op != '/') {
            break;
        }
        
        p->pos++; // skip operator
        double right = parse_factor(p);
        
        if (op == '*') {
            result *= right;
        } else { // op == '/'
            if (right == 0) {
                return NAN; // Division by zero
            }
            result /= right;
        }
    }
    
    return result;
}

static double parse_expression(parser_t *p)
{
    double result = parse_term(p);
    
    while (p->pos < p->len) {
        skip_whitespace(p);
        if (p->pos >= p->len) break;
        
        char op = p->expr[p->pos];
        if (op != '+' && op != '-') {
            break;
        }
        
        p->pos++; // skip operator
        double right = parse_term(p);
        
        if (op == '+') {
            result += right;
        } else { // op == '-'
            result -= right;
        }
    }
    
    return result;
}

int calculator_evaluate(const char *expression, double *result)
{
    if (!expression || !result) {
        return -EINVAL;
    }
    
    parser_t parser = {
        .expr = expression,
        .pos = 0,
        .len = strlen(expression)
    };
    
    *result = parse_expression(&parser);
    
    // Check for parse errors
    if (isnan(*result) || isinf(*result)) {
        return -EDOM;
    }
    
    return 0;
}
