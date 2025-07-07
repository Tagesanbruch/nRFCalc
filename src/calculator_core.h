/*
 * Calculator Core - Compatibility wrapper for new modular structure
 * 
 * This file provides backward compatibility by including the new
 * modular calculator implementation headers.
 */

#ifndef CALCULATOR_CORE_H
#define CALCULATOR_CORE_H

#include "state/calculator_state.h"
#include "ui/calculator_ui.h"
#include "math/expression_evaluator.h"

// Type alias for backward compatibility
typedef calculator_t calculator;

// Function wrappers for backward compatibility
// (The actual implementations are in the respective modules)

#endif /* CALCULATOR_CORE_H */
