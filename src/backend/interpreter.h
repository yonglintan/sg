#ifndef sg_interpreter_h
#define sg_interpreter_h

#include "../ast/expr.h"
#include "../ast/stmt.h"
#include "../runtime/object.h"
#include "../frontend/scanner.h"
#include <stdbool.h>

// Structure to hold runtime error info (optional but good practice)
// typedef struct {
//     Token token; // Token near the error
//     const char* message;
// } RuntimeErrorInfo;

// Initialize the interpreter (creates global environment)
void initInterpreter();

// Free interpreter resources (frees global environment)
void freeInterpreter();

// Interpret a list of statements
// Returns true on success, false if a runtime error occurred.
void interpretStatements(StmtList* statements);

// Function to report a runtime error
// Uses varargs for formatting like printf
void runtimeError(Token* token, const char* format, ...);

// Check if a runtime error has occurred during interpretation
bool hadRuntimeError();

// Reset the runtime error flag (e.g., for REPL use)
void resetRuntimeError();

#endif 