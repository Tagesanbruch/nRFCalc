/*
 * Expression Evaluator Implementation
 * Shunting-yard Algorithm for Mathematical Expression Parsing
 */

#include "expression_evaluator.h"
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

// Define math constants if not available
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_E
#define M_E 2.7182818284590452354
#endif

LOG_MODULE_REGISTER(expression_evaluator, LOG_LEVEL_INF);

// Error codes
#define ERR_SYNTAX_ERROR        -1
#define ERR_DIVISION_BY_ZERO    -2
#define ERR_DOMAIN_ERROR        -3
#define ERR_OVERFLOW            -4
#define ERR_STACK_OVERFLOW      -5
#define ERR_UNKNOWN_FUNCTION    -6
#define ERR_MISMATCHED_PARENS   -7

// Function name mapping
static const char* function_names[FUNC_COUNT] = {
    "sin", "cos", "tan",
    "asin", "acos", "atan",
    "log", "ln", "log10",
    "sqrt", "abs", "exp",
    "sinh", "cosh", "tanh",
    "!"
};

// Function name patterns for parsing
static const struct {
    const char* pattern;
    function_type_t type;
} function_patterns[] = {
    {"sin⁻¹", FUNC_ASIN}, {"cos⁻¹", FUNC_ACOS}, {"tan⁻¹", FUNC_ATAN},
    {"sin", FUNC_SIN}, {"cos", FUNC_COS}, {"tan", FUNC_TAN},
    {"sinh", FUNC_SINH}, {"cosh", FUNC_COSH}, {"tanh", FUNC_TANH},
    {"log", FUNC_LOG}, {"ln", FUNC_LN}, {"log10", FUNC_LOG10},
    {"sqrt", FUNC_SQRT}, {"abs", FUNC_ABS}, {"exp", FUNC_EXP}
};

// Constant patterns
static const struct {
    const char* pattern;
    constant_type_t type;
    double value;
} constant_patterns[] = {
    {"π", CONST_PI, M_PI},
    {"pi", CONST_PI, M_PI},
    {"e", CONST_E, M_E}
};

// Variable patterns
static const struct {
    const char* pattern;
    variable_type_t type;
} variable_patterns[] = {
    {"Ans", VAR_ANS},
    {"X", VAR_X}, {"Y", VAR_Y},
    {"A", VAR_A}, {"B", VAR_B}, {"C", VAR_C}, {"D", VAR_D},
    {"M", VAR_M}
};

int get_operator_precedence(char op)
{
    switch (op) {
        case '+':
        case '-':
            return 1;
        case '*':
        case '/':
            return 2;
        case '^':
            return 3;
        default:
            return 0;
    }
}

bool is_right_associative(char op)
{
    return (op == '^');
}

const char* get_function_name(function_type_t func)
{
    if (func < FUNC_COUNT) {
        return function_names[func];
    }
    return "unknown";
}

double get_constant_value(constant_type_t constant)
{
    switch (constant) {
        case CONST_PI:
            return M_PI;
        case CONST_E:
            return M_E;
        default:
            return 0.0;
    }
}

// Get variable value from storage
static double get_variable_value(variable_type_t var, const variable_storage_t *storage)
{
    switch (var) {
        case VAR_ANS: return storage->ans;
        case VAR_X: return storage->x;
        case VAR_Y: return storage->y;
        case VAR_A: return storage->a;
        case VAR_B: return storage->b;
        case VAR_C: return storage->c;
        case VAR_D: return storage->d;
        case VAR_M: return storage->m;
        default: return 0.0;
    }
}

// Calculate factorial
static double factorial(double n)
{
    if (n < 0 || n != floor(n) || n > 170) {
        return NAN; // Invalid input or would overflow
    }
    
    double result = 1.0;
    for (int i = 2; i <= (int)n; i++) {
        result *= i;
    }
    return result;
}

// Apply mathematical function
static double apply_function(function_type_t func, double arg, bool deg_mode)
{
    // Convert angle to radians if in degree mode
    double angle_arg = arg;
    if (deg_mode && (func == FUNC_SIN || func == FUNC_COS || func == FUNC_TAN)) {
        angle_arg = arg * M_PI / 180.0;
    }
    
    double result;
    switch (func) {
        case FUNC_SIN: result = sin(angle_arg); break;
        case FUNC_COS: result = cos(angle_arg); break;
        case FUNC_TAN: result = tan(angle_arg); break;
        case FUNC_ASIN: 
            result = asin(arg);
            if (deg_mode) result = result * 180.0 / M_PI;
            break;
        case FUNC_ACOS:
            result = acos(arg);
            if (deg_mode) result = result * 180.0 / M_PI;
            break;
        case FUNC_ATAN:
            result = atan(arg);
            if (deg_mode) result = result * 180.0 / M_PI;
            break;
        case FUNC_LOG: result = log10(arg); break;
        case FUNC_LN: result = log(arg); break;
        case FUNC_LOG10: result = log10(arg); break;
        case FUNC_SQRT: result = sqrt(arg); break;
        case FUNC_ABS: result = fabs(arg); break;
        case FUNC_EXP: result = exp(arg); break;
        case FUNC_SINH: result = sinh(arg); break;
        case FUNC_COSH: result = cosh(arg); break;
        case FUNC_TANH: result = tanh(arg); break;
        case FUNC_FACTORIAL: result = factorial(arg); break;
        default: return NAN;
    }
    
    return result;
}

// Tokenizer helper functions
static int skip_whitespace(const char *expr, int pos)
{
    while (expr[pos] && isspace(expr[pos])) {
        pos++;
    }
    return pos;
}

static int parse_number(const char *expr, int pos, double *number)
{
    char *endptr;
    *number = strtod(&expr[pos], &endptr);
    return endptr - expr;
}

static int parse_function(const char *expr, int pos, function_type_t *function)
{
    for (int i = 0; i < sizeof(function_patterns) / sizeof(function_patterns[0]); i++) {
        int len = strlen(function_patterns[i].pattern);
        if (strncmp(&expr[pos], function_patterns[i].pattern, len) == 0) {
            *function = function_patterns[i].type;
            return pos + len;
        }
    }
    return -1; // Not found
}

static int parse_constant(const char *expr, int pos, constant_type_t *constant)
{
    for (int i = 0; i < sizeof(constant_patterns) / sizeof(constant_patterns[0]); i++) {
        int len = strlen(constant_patterns[i].pattern);
        if (strncmp(&expr[pos], constant_patterns[i].pattern, len) == 0) {
            *constant = constant_patterns[i].type;
            return pos + len;
        }
    }
    return -1; // Not found
}

static int parse_variable(const char *expr, int pos, variable_type_t *variable)
{
    for (int i = 0; i < sizeof(variable_patterns) / sizeof(variable_patterns[0]); i++) {
        int len = strlen(variable_patterns[i].pattern);
        if (strncmp(&expr[pos], variable_patterns[i].pattern, len) == 0) {
            *variable = variable_patterns[i].type;
            return pos + len;
        }
    }
    return -1; // Not found
}

// Tokenize expression into tokens
static int tokenize_expression(const char *expression, token_t *tokens, int max_tokens)
{
    int pos = 0;
    int token_count = 0;
    int len = strlen(expression);
    bool expect_number = true; // Expect number or unary operator at start
    
    while (pos < len && token_count < max_tokens - 1) {
        pos = skip_whitespace(expression, pos);
        if (pos >= len) break;
        
        char ch = expression[pos];
        
        // Numbers
        if (isdigit(ch) || ch == '.') {
            double number;
            int new_pos = parse_number(expression, pos, &number);
            if (new_pos > pos) {
                tokens[token_count].type = TOKEN_NUMBER;
                tokens[token_count].value.number = number;
                token_count++;
                pos = new_pos;
                expect_number = false;
                continue;
            }
        }
        
        // Functions
        function_type_t function;
        int func_pos = parse_function(expression, pos, &function);
        if (func_pos > 0) {
            tokens[token_count].type = TOKEN_FUNCTION;
            tokens[token_count].value.function = function;
            token_count++;
            pos = func_pos;
            expect_number = true;
            continue;
        }
        
        // Constants
        constant_type_t constant;
        int const_pos = parse_constant(expression, pos, &constant);
        if (const_pos > 0) {
            tokens[token_count].type = TOKEN_CONSTANT;
            tokens[token_count].value.constant = constant;
            token_count++;
            pos = const_pos;
            expect_number = false;
            continue;
        }
        
        // Variables
        variable_type_t variable;
        int var_pos = parse_variable(expression, pos, &variable);
        if (var_pos > 0) {
            tokens[token_count].type = TOKEN_VARIABLE;
            tokens[token_count].value.variable = variable;
            token_count++;
            pos = var_pos;
            expect_number = false;
            continue;
        }
        
        // Operators and parentheses
        switch (ch) {
            case '+':
            case '*':
            case '/':
            case '^':
                if (expect_number && ch != '-') {
                    return ERR_SYNTAX_ERROR;
                }
                tokens[token_count].type = TOKEN_OPERATOR;
                tokens[token_count].value.operator = ch;
                token_count++;
                pos++;
                expect_number = true;
                break;
                
            case '-':
                if (expect_number) {
                    // Unary minus
                    tokens[token_count].type = TOKEN_UNARY_MINUS;
                    token_count++;
                } else {
                    // Binary minus
                    tokens[token_count].type = TOKEN_OPERATOR;
                    tokens[token_count].value.operator = ch;
                    token_count++;
                }
                pos++;
                expect_number = true;
                break;
                
            case '(':
                tokens[token_count].type = TOKEN_LEFT_PAREN;
                token_count++;
                pos++;
                expect_number = true;
                break;
                
            case ')':
                if (expect_number) {
                    return ERR_SYNTAX_ERROR;
                }
                tokens[token_count].type = TOKEN_RIGHT_PAREN;
                token_count++;
                pos++;
                expect_number = false;
                break;
                
            case '!':
                // Factorial operator (postfix)
                if (expect_number) {
                    return ERR_SYNTAX_ERROR;
                }
                tokens[token_count].type = TOKEN_FUNCTION;
                tokens[token_count].value.function = FUNC_FACTORIAL;
                token_count++;
                pos++;
                expect_number = false;
                break;
                
            default:
                LOG_ERR("Unknown character: %c at position %d", ch, pos);
                return ERR_SYNTAX_ERROR;
        }
    }
    
    // Add end marker
    tokens[token_count].type = TOKEN_END;
    return token_count;
}

int parse_expression_to_rpn(const char *expression, rpn_queue_t *rpn_queue)
{
    token_t tokens[MAX_TOKENS];
    token_t operator_stack[MAX_TOKENS];
    int stack_top = -1;
    
    // Tokenize the expression
    int token_count = tokenize_expression(expression, tokens, MAX_TOKENS);
    if (token_count < 0) {
        return token_count; // Error code
    }
    
    rpn_queue->count = 0;
    
    // Shunting-yard algorithm
    for (int i = 0; i < token_count; i++) {
        token_t *token = &tokens[i];
        
        switch (token->type) {
            case TOKEN_NUMBER:
            case TOKEN_CONSTANT:
            case TOKEN_VARIABLE:
                // Numbers, constants, and variables go directly to output
                if (rpn_queue->count >= MAX_TOKENS) {
                    return ERR_STACK_OVERFLOW;
                }
                rpn_queue->tokens[rpn_queue->count++] = *token;
                break;
                
            case TOKEN_FUNCTION:
                // Functions go to operator stack
                if (stack_top >= MAX_TOKENS - 1) {
                    return ERR_STACK_OVERFLOW;
                }
                operator_stack[++stack_top] = *token;
                break;
                
            case TOKEN_OPERATOR:
            case TOKEN_UNARY_MINUS: {
                char op = (token->type == TOKEN_UNARY_MINUS) ? '-' : token->value.operator;
                int precedence = get_operator_precedence(op);
                bool right_assoc = is_right_associative(op);
                
                // Pop operators with higher or equal precedence
                while (stack_top >= 0 && 
                       operator_stack[stack_top].type != TOKEN_LEFT_PAREN &&
                       ((operator_stack[stack_top].type == TOKEN_OPERATOR &&
                         get_operator_precedence(operator_stack[stack_top].value.operator) > precedence) ||
                        (operator_stack[stack_top].type == TOKEN_OPERATOR &&
                         get_operator_precedence(operator_stack[stack_top].value.operator) == precedence &&
                         !right_assoc) ||
                        operator_stack[stack_top].type == TOKEN_FUNCTION ||
                        operator_stack[stack_top].type == TOKEN_UNARY_MINUS)) {
                    
                    if (rpn_queue->count >= MAX_TOKENS) {
                        return ERR_STACK_OVERFLOW;
                    }
                    rpn_queue->tokens[rpn_queue->count++] = operator_stack[stack_top--];
                }
                
                // Push current operator
                if (stack_top >= MAX_TOKENS - 1) {
                    return ERR_STACK_OVERFLOW;
                }
                operator_stack[++stack_top] = *token;
                break;
            }
            
            case TOKEN_LEFT_PAREN:
                // Left parenthesis goes to operator stack
                if (stack_top >= MAX_TOKENS - 1) {
                    return ERR_STACK_OVERFLOW;
                }
                operator_stack[++stack_top] = *token;
                break;
                
            case TOKEN_RIGHT_PAREN:
                // Pop until left parenthesis
                while (stack_top >= 0 && operator_stack[stack_top].type != TOKEN_LEFT_PAREN) {
                    if (rpn_queue->count >= MAX_TOKENS) {
                        return ERR_STACK_OVERFLOW;
                    }
                    rpn_queue->tokens[rpn_queue->count++] = operator_stack[stack_top--];
                }
                
                if (stack_top < 0) {
                    return ERR_MISMATCHED_PARENS;
                }
                
                // Pop the left parenthesis
                stack_top--;
                
                // If there's a function on top, pop it too
                if (stack_top >= 0 && operator_stack[stack_top].type == TOKEN_FUNCTION) {
                    if (rpn_queue->count >= MAX_TOKENS) {
                        return ERR_STACK_OVERFLOW;
                    }
                    rpn_queue->tokens[rpn_queue->count++] = operator_stack[stack_top--];
                }
                break;
                
            case TOKEN_END:
                // End of input - should not happen here
                break;
        }
    }
    
    // Pop remaining operators
    while (stack_top >= 0) {
        if (operator_stack[stack_top].type == TOKEN_LEFT_PAREN) {
            return ERR_MISMATCHED_PARENS;
        }
        if (rpn_queue->count >= MAX_TOKENS) {
            return ERR_STACK_OVERFLOW;
        }
        rpn_queue->tokens[rpn_queue->count++] = operator_stack[stack_top--];
    }
    
    return 0; // Success
}

int evaluate_rpn(const rpn_queue_t *rpn_queue, const eval_context_t *context, double *result)
{
    double stack[MAX_TOKENS];
    int stack_top = -1;
    
    for (int i = 0; i < rpn_queue->count; i++) {
        const token_t *token = &rpn_queue->tokens[i];
        
        switch (token->type) {
            case TOKEN_NUMBER:
                // Push number onto stack
                if (stack_top >= MAX_TOKENS - 1) {
                    return ERR_STACK_OVERFLOW;
                }
                stack[++stack_top] = token->value.number;
                break;
                
            case TOKEN_CONSTANT:
                // Push constant value onto stack
                if (stack_top >= MAX_TOKENS - 1) {
                    return ERR_STACK_OVERFLOW;
                }
                stack[++stack_top] = get_constant_value(token->value.constant);
                break;
                
            case TOKEN_VARIABLE:
                // Push variable value onto stack
                if (stack_top >= MAX_TOKENS - 1) {
                    return ERR_STACK_OVERFLOW;
                }
                stack[++stack_top] = get_variable_value(token->value.variable, &context->variables);
                break;
                
            case TOKEN_OPERATOR: {
                // Pop two operands and apply operator
                if (stack_top < 1) {
                    return ERR_SYNTAX_ERROR;
                }
                
                double b = stack[stack_top--];
                double a = stack[stack_top--];
                double op_result;
                
                switch (token->value.operator) {
                    case '+': op_result = a + b; break;
                    case '-': op_result = a - b; break;
                    case '*': op_result = a * b; break;
                    case '/':
                        if (fabs(b) < 1e-15) {
                            return ERR_DIVISION_BY_ZERO;
                        }
                        op_result = a / b;
                        break;
                    case '^': op_result = pow(a, b); break;
                    default: return ERR_SYNTAX_ERROR;
                }
                
                if (!isfinite(op_result)) {
                    return ERR_OVERFLOW;
                }
                
                stack[++stack_top] = op_result;
                break;
            }
            
            case TOKEN_UNARY_MINUS:
                // Apply unary minus
                if (stack_top < 0) {
                    return ERR_SYNTAX_ERROR;
                }
                stack[stack_top] = -stack[stack_top];
                break;
                
            case TOKEN_FUNCTION:
                // Apply function
                if (stack_top < 0) {
                    return ERR_SYNTAX_ERROR;
                }
                
                double arg = stack[stack_top--];
                double func_result = apply_function(token->value.function, arg, context->deg_mode);
                
                if (!isfinite(func_result)) {
                    return ERR_DOMAIN_ERROR;
                }
                
                stack[++stack_top] = func_result;
                break;
                
            default:
                return ERR_SYNTAX_ERROR;
        }
    }
    
    // Should have exactly one result on stack
    if (stack_top != 0) {
        return ERR_SYNTAX_ERROR;
    }
    
    *result = stack[0];
    return 0;
}

int evaluate_expression(const char *expression, const eval_context_t *context, double *result)
{
    rpn_queue_t rpn_queue;
    
    // Parse expression to RPN
    int parse_result = parse_expression_to_rpn(expression, &rpn_queue);
    if (parse_result < 0) {
        LOG_ERR("Failed to parse expression: %s (error %d)", expression, parse_result);
        return parse_result;
    }
    
    // Evaluate RPN
    int eval_result = evaluate_rpn(&rpn_queue, context, result);
    if (eval_result < 0) {
        LOG_ERR("Failed to evaluate RPN (error %d)", eval_result);
        return eval_result;
    }
    
    LOG_INF("Evaluated '%s' = %g", expression, *result);
    return 0;
}
