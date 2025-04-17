#ifndef sg_environment_h
#define sg_environment_h

#include "object.h"  
#include "scanner.h"

// forward declare environment struct for the pointer in itself
typedef struct Environment Environment;

// using a simple dynamic array for entries
// alternatives: hash table (better performance), linked list (simpler?).
typedef struct {
    char* name;  // Variable name (key)
    Value value; // Variable value
} Entry;

struct Environment {
    int count;
    int capacity;
    Entry* entries; // dynamic array of entries
    Environment* enclosing; // pointer to outer scope's environment, NULL if global
};

// initialize a new top-level (global) environment
Environment* newEnvironment();

// initialize a new nested environment linked to an outer one
Environment* newEnclosedEnvironment(Environment* enclosing);

// free an environment and its entries (but not recursively freeing enclosing)
void freeEnvironment(Environment* environment);

// define (or re-define) a variable in the *current* environment scope.
// name ownership is taken (environment will free it).
bool environmentDefine(Environment* environment, const char* name, Value value);

// get a variable's value. checks current scope then enclosing scopes recursively.
// returns true if found (value copied to *outValue), false otherwise.
// caller should check return value; this function doesn't report runtime errors.
bool environmentGet(Environment* environment, Token* nameToken, Value* outValue);

// assign a value to an *existing* variable. checks current scope then enclosing recursively.
// returns true if assignment successful, false if variable not found.
// caller should check return value; this function doesn't report runtime errors.
bool environmentAssign(Environment* environment, Token* nameToken, Value value);


#endif 