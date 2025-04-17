#ifndef sg_object_h
#define sg_object_h

#include <stdbool.h>

// stuff supported for now
typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUMBER,
    // TODO: VAL_OBJ 
} ValueType;

// typedef struct Obj Obj;

// value structure - represents any runtime value
typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
        // Obj* obj; // pointer to heap-allocated object
    } as;
} Value;

// Macros to check the type of a Value
#define IS_BOOL(value)    ((value).type == VAL_BOOL)
#define IS_NIL(value)     ((value).type == VAL_NIL)
#define IS_NUMBER(value)  ((value).type == VAL_NUMBER)
// #define IS_OBJ(value)     ((value).type == VAL_OBJ)

// Macros to get the C value from a Value struct
#define AS_BOOL(value)    ((value).as.boolean)
#define AS_NUMBER(value)  ((value).as.number)
// #define AS_OBJ(value)     ((value).as.obj)

// Macros to create Value structs from C values
#define BOOL_VAL(value)   ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VAL           ((Value){VAL_NIL, {.number = 0}}) // .number is arbitrary for NIL
#define NUMBER_VAL(value) ((Value){VAL_NUMBER, {.number = value}})
// #define OBJ_VAL(object)   ((Value){VAL_OBJ, {.obj = (Obj*)object}})

// print value (for debugging/printing)
void printValue(Value value);

// check equality (needed later for interpreter)
bool valuesEqual(Value a, Value b);

#endif 