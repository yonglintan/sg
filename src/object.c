#include "object.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stmt.h"
#include "environment.h"

#include "memory.h"

Obj* allocateObject(size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    if (object == NULL) {
        fprintf(stderr, "Fatal: Memory allocation failed for object!\n");
        exit(1);
    }
    object->type = type;

    return object;
}

ObjString* copyString(const char* chars, int length) {
    // Allocate memory for the ObjString struct itself PLUS the character array
    // (+1 for null terminator)
    char* heapChars = ALLOCATE(char, length + 1);
    if (heapChars == NULL) return NULL;
    memcpy(heapChars, chars, length);
    heapChars[length] = '\0';

    ObjString* string =
        (ObjString*)allocateObject(sizeof(ObjString), OBJ_STRING);

    if (string == NULL) {
        // failed, just free
        FREE(char, heapChars);
        return NULL;
    }

    string->length = length;
    string->chars = heapChars;

    return string;
}

void printObject(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
        case OBJ_FUNCTION: {
            ObjFunction* function = AS_FUNCTION(value);
            printf("<fn %.*s>", 
                   function->declaration->as.function.name.length,
                   function->declaration->as.function.name.start);
            break;
        }
        case OBJ_NATIVE:
            printf("<native fn>");
            break;
    }
}

void printValue(Value value) {
    switch (value.type) {
        case VAL_BOOL:
            printf(AS_BOOL(value) ? "correct" : "wrong");
            break;
        case VAL_NIL:
            printf("nil");
            break;
        case VAL_NUMBER:
            printf("%g", AS_NUMBER(value));
            break;
        case VAL_OBJ:
            printObject(value);
            break;
    }
}

bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_BOOL:
            return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:
            return true;
        case VAL_NUMBER:
            return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ: {
            // For now, only strings are objects. Compare them by content.
            // If we add other object types, this needs more logic.
            if (IS_STRING(a) && IS_STRING(b)) {
                ObjString* aString = AS_STRING(a);
                ObjString* bString = AS_STRING(b);
                // Compare length first for quick check, then content
                return aString->length == bString->length &&
                       memcmp(aString->chars, bString->chars,
                              aString->length) == 0;
            }
            return false;
        }
        default:
            return false;
    }
}

ObjFunction* newFunction(Stmt* declaration, Environment* closure) {
    ObjFunction* function = (ObjFunction*)allocateObject(sizeof(ObjFunction), OBJ_FUNCTION);
    if (function == NULL) return NULL;
    function->declaration = declaration;
    function->closure = closure;
    function->arity = declaration->as.function.param_count;
    return function;
}

ObjNative* newNative(int arity, Value (*function)(struct Interpreter*, int, Value*)) {
    ObjNative* native = (ObjNative*)allocateObject(sizeof(ObjNative), OBJ_NATIVE);
    native->arity = arity;
    native->function = function;
    return native;
}