#include <stdio.h>
#include <string.h>
#include "object.h"

void printValue(Value value) {
    switch (value.type) {
        case VAL_BOOL:
            printf(AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NIL:
            printf("nil");
            break;
        case VAL_NUMBER:
            printf("%g", AS_NUMBER(value)); // %g is nice for floats/doubles
            break;
        // Later: Handle VAL_OBJ for strings, etc.
        // case VAL_OBJ: printObject(value); break;
    }
}

bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) return false;

    switch (a.type) {
        case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:    return true; // nil is only equal to nil
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        // Later: Handle VAL_OBJ comparison (reference or value equality?)
        // case VAL_OBJ: return AS_OBJ(a) == AS_OBJ(b); // For now, reference equality
        default:         return false; // Unreachable
    }
} 