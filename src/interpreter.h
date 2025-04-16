#ifndef interpreter_h
#define interpreter_h

#include "expr.h"

// Runtime error structure
typedef struct {
    const char* message;
    Token token;
} RuntimeError;

// Interpret an expression and return the result
Expr* interpret(Expr* expression);

// Check if there was a runtime error
bool hadRuntimeError(void);

// Reset the runtime error flag
void resetRuntimeError(void);

#endif 