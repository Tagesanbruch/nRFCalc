/*
 * Expression Evaluator - Shunting-yard Algorithm Implementation
 * 
 * This module implements the Shunting-yard algorithm for parsing infix
 * mathematical expressions into postfix notation (RPN), which can then
 * be easily evaluated using a stack.
 * 
 * Supports:
 * - Basic arithmetic operators (+, -, *, /, ^)
 * - Mathematical functions (sin, cos, tan, log, ln, sqrt, etc.)
 * - Constants (π, e)
 * - Parentheses
 * - Unary operators (negative numbers)
 */

#ifndef EXPRESSION_EVALUATOR_H
#define EXPRESSION_EVALUATOR_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_TOKENS 64
#define MAX_EXPRESSION_LENGTH 128

/**
 * @brief Token types for expression parsing
 */
typedef enum {
    TOKEN_NUMBER,       // Numeric value
    TOKEN_OPERATOR,     // Binary operator (+, -, *, /, ^)
    TOKEN_FUNCTION,     // Mathematical function (sin, cos, etc.)
    TOKEN_CONSTANT,     // Mathematical constant (π, e)
    TOKEN_VARIABLE,     // Variable (Ans, X, Y, etc.)
    TOKEN_LEFT_PAREN,   // Left parenthesis
    TOKEN_RIGHT_PAREN,  // Right parenthesis
    TOKEN_UNARY_MINUS,  // Unary minus operator
    TOKEN_END           // End of expression marker
} token_type_t;

/**
 * @brief Mathematical function types
 */
typedef enum {
    FUNC_SIN, FUNC_COS, FUNC_TAN,
    FUNC_ASIN, FUNC_ACOS, FUNC_ATAN,
    FUNC_LOG, FUNC_LN, FUNC_LOG10,
    FUNC_SQRT, FUNC_ABS, FUNC_EXP,
    FUNC_SINH, FUNC_COSH, FUNC_TANH,
    FUNC_FACTORIAL,
    FUNC_COUNT
} function_type_t;

/**
 * @brief Mathematical constant types
 */
typedef enum {
    CONST_PI,       // π
    CONST_E,        // e
    CONST_COUNT
} constant_type_t;

/**
 * @brief Variable types
 */
typedef enum {
    VAR_ANS,        // Last result
    VAR_X, VAR_Y,   // Variables X, Y
    VAR_A, VAR_B, VAR_C, VAR_D,  // Memory variables
    VAR_M,          // Memory M
    VAR_COUNT
} variable_type_t;

/**
 * @brief Token structure for expression parsing
 */
typedef struct {
    token_type_t type;
    union {
        double number;
        char operator;
        function_type_t function;
        constant_type_t constant;
        variable_type_t variable;
    } value;
} token_t;

/**
 * @brief RPN (Reverse Polish Notation) queue for evaluation
 */
typedef struct {
    token_t tokens[MAX_TOKENS];
    int count;
} rpn_queue_t;

/**
 * @brief Variable storage for expression evaluation
 */
typedef struct {
    double ans;         // Last result
    double x, y;        // Variables X, Y
    double a, b, c, d;  // Memory variables A-D
    double m;           // Memory M
} variable_storage_t;

/**
 * @brief Evaluation context
 */
typedef struct {
    variable_storage_t variables;
    bool deg_mode;      // True for degrees, false for radians
} eval_context_t;

/**
 * @brief Parse infix expression to RPN using Shunting-yard algorithm
 * @param expression Input mathematical expression string
 * @param rpn_queue Output RPN token queue
 * @return 0 on success, negative error code on failure
 */
int parse_expression_to_rpn(const char *expression, rpn_queue_t *rpn_queue);

/**
 * @brief Evaluate RPN token queue
 * @param rpn_queue RPN tokens to evaluate
 * @param context Evaluation context (variables, angle mode)
 * @param result Pointer to store the result
 * @return 0 on success, negative error code on failure
 */
int evaluate_rpn(const rpn_queue_t *rpn_queue, const eval_context_t *context, double *result);

/**
 * @brief High-level expression evaluation function
 * @param expression Input mathematical expression string
 * @param context Evaluation context
 * @param result Pointer to store the result
 * @return 0 on success, negative error code on failure
 */
int evaluate_expression(const char *expression, const eval_context_t *context, double *result);

/**
 * @brief Get operator precedence
 * @param op Operator character
 * @return Precedence value (higher = higher precedence)
 */
int get_operator_precedence(char op);

/**
 * @brief Check if operator is right-associative
 * @param op Operator character
 * @return True if right-associative, false if left-associative
 */
bool is_right_associative(char op);

/**
 * @brief Get function name string
 * @param func Function type
 * @return Function name string
 */
const char* get_function_name(function_type_t func);

/**
 * @brief Get constant value
 * @param constant Constant type
 * @return Constant value
 */
double get_constant_value(constant_type_t constant);

#endif /* EXPRESSION_EVALUATOR_H */
