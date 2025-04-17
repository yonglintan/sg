#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include "expr.h"
#include "memory.h"

static Expr* allocateExpr(ExprType type) {
    Expr* expr = ALLOCATE(Expr, 1);
    if (expr != NULL) {
        expr->type = type;
    }
    return expr;
}

Expr* newAssignExpr(Token name, Expr* value) {
    Expr* expr = allocateExpr(EXPR_ASSIGN);
    if (expr == NULL) return NULL;
    expr->as.assign.name = name;
    expr->as.assign.value = value;
    return expr;
}

Expr* newBinaryExpr(Expr* left, Token operator, Expr* right) {
    Expr* expr = allocateExpr(EXPR_BINARY);
    if (expr == NULL) return NULL;
    expr->as.binary.left = left;
    expr->as.binary.operator = operator;
    expr->as.binary.right = right;
    return expr;
}

Expr* newGroupingExpr(Expr* expression) {
    Expr* expr = allocateExpr(EXPR_GROUPING);
     if (expr == NULL) return NULL;
    expr->as.grouping.expression = expression;
    return expr;
}

Expr* newLiteralNumberExpr(double value) {
    Expr* expr = allocateExpr(EXPR_LITERAL);
     if (expr == NULL) return NULL;
    expr->as.literal.type = TOKEN_NUMBER;
    expr->as.literal.value.number = value;
    return expr;
}

Expr* newLiteralBooleanExpr(bool value) {
    Expr* expr = allocateExpr(EXPR_LITERAL);
    if (expr == NULL) return NULL;
    expr->as.literal.type = value ? TOKEN_TRUE : TOKEN_FALSE;
    expr->as.literal.value.boolean = value;
    return expr;
}

Expr* newLiteralStringExpr(char* value) {
    Expr* expr = allocateExpr(EXPR_LITERAL);
    if (expr == NULL) return NULL;
    expr->as.literal.type = TOKEN_STRING;
    // copy for now, can look into immutability and stuff later but not impt now
    char* valueCopy = ALLOCATE(char, strlen(value) + 1);
    if (valueCopy == NULL) {
        FREE(Expr, expr);
        return NULL;
    }
    strcpy(valueCopy, value);
    expr->as.literal.value.string = valueCopy;
    return expr;
}

Expr* newLiteralNilExpr() {
    Expr* expr = allocateExpr(EXPR_LITERAL);
    if (expr == NULL) return NULL;
    expr->as.literal.type = TOKEN_NIL;
    return expr;
}

Expr* newUnaryExpr(Token operator, Expr* right) {
    Expr* expr = allocateExpr(EXPR_UNARY);
    if (expr == NULL) return NULL;
    expr->as.unary.operator = operator;
    expr->as.unary.right = right;
    return expr;
}

Expr* newVariableExpr(Token name) {
    Expr* expr = allocateExpr(EXPR_VARIABLE);
    if (expr == NULL) return NULL;
    expr->as.variable.name = name;
    return expr;
}


void freeExpr(Expr* expr) {
    if (expr == NULL) return;

    switch (expr->type) {
        case EXPR_ASSIGN: {
            // free the RHS expression, 
            // IMPORTANT: but not the name token (owned by scanner/parser)
            freeExpr(expr->as.assign.value);
            break;
        }
        case EXPR_BINARY: {
            freeExpr(expr->as.binary.left);
            freeExpr(expr->as.binary.right);
            break;
        }
        case EXPR_GROUPING: {
            freeExpr(expr->as.grouping.expression);
            break;
        }
        case EXPR_LITERAL: {
            // If it's a string literal, free the copied string
            if (expr->as.literal.type == TOKEN_STRING) {
                FREE(char, expr->as.literal.value.string);
            }
            // Other literals (number, bool, nil) don't need freeing
            break;
        }
        case EXPR_UNARY: {
            freeExpr(expr->as.unary.right);
            break;
        }
        case EXPR_VARIABLE: {
            // The token itself is not owned by the expression node
            break;
        }
        // Add default case to handle potential future types or errors
        default: 
            // Optionally report an error or log unknown type
            // fprintf(stderr, "Warning: freeExpr called on unknown type %d\n", expr->type);
            break; 
    }

    // Finally, free the expression node itself
    FREE(Expr, expr);
}

typedef struct {
    char* buffer;
    int capacity;
    int length;
} StringBuilder;

StringBuilder* newStringBuilder() {
    StringBuilder* sb = (StringBuilder*)malloc(sizeof(StringBuilder));
    sb->capacity = 64;
    sb->buffer = (char*)malloc(sb->capacity);
    sb->length = 0;
    sb->buffer[0] = '\0';
    return sb;
}

void appendString(StringBuilder* sb, const char* str) {
    int len = strlen(str);
    
    // make sure we have enough space
    if (sb->capacity < sb->length + len + 1) {
        int newCapacity = (sb->capacity == 0) ? 64 : sb->capacity * 2;
        while (newCapacity < sb->length + len + 1) {
            newCapacity *= 2;
        }
        
        sb->buffer = (char*)realloc(sb->buffer, newCapacity);
        sb->capacity = newCapacity;
    }
    
    // once done, append the string
    strcpy(sb->buffer + sb->length, str);
    sb->length += len;
}

void freeStringBuilder(StringBuilder* sb) {
    free(sb->buffer);
    free(sb);
}


void printExprInternal(StringBuilder* sb, Expr* expr);


void parenthesize(StringBuilder* sb, const char* name, int count, ...) {
    va_list args;
    
    appendString(sb, "(");
    appendString(sb, name);
    
    va_start(args, count);
    for (int i = 0; i < count; i++) {
        appendString(sb, " ");
        Expr* expr = va_arg(args, Expr*);
        printExprInternal(sb, expr);
    }
    va_end(args);
    
    appendString(sb, ")");
}


void printExprInternal(StringBuilder* sb, Expr* expr) {
    switch (expr->type) {
        case EXPR_BINARY: {
            char op[2] = {0};
            op[0] = *expr->as.binary.operator.start;
            parenthesize(sb, op, 2, expr->as.binary.left, expr->as.binary.right);
            break;
        }
        case EXPR_GROUPING: {
            parenthesize(sb, "group", 1, expr->as.grouping.expression);
            break;
        }
        case EXPR_LITERAL: {
            switch (expr->as.literal.type) {
                case TOKEN_NIL:
                    appendString(sb, "nil");
                    break;
                case TOKEN_TRUE:
                case TOKEN_FALSE:
                    appendString(sb, expr->as.literal.value.boolean ? "true" : "false");
                    break;
                case TOKEN_NUMBER: {
                    char buffer[32];
                    snprintf(buffer, sizeof(buffer), "%g", expr->as.literal.value.number);
                    appendString(sb, buffer);
                    break;
                }
                case TOKEN_STRING:
                    appendString(sb, expr->as.literal.value.string);
                    break;
                default:
                    appendString(sb, "unknown literal");
                    break;
            }
            break;
        }
        case EXPR_UNARY: {
            
            char op[2] = {0};
            op[0] = *expr->as.unary.operator.start;
            parenthesize(sb, op, 1, expr->as.unary.right);
            break;
        }
    }
}


// --- Debugging: AST Printer ---
static void printExprRecursive(Expr* expr, char* buffer, size_t* pos, size_t capacity) {
    if (expr == NULL || *pos >= capacity - 1) return;

    // Simplified operator printing for brevity in this example
    const char* opStr = "op?"; 

    int written = 0;
    switch (expr->type) {
        case EXPR_ASSIGN: {
             Token name = expr->as.assign.name;
             // Use snprintf safely
             written = snprintf(buffer + *pos, capacity - *pos, "(= %.*s ", name.length, name.start);
             if (written < 0 || (size_t)written >= capacity - *pos) return; // Check for error/truncation
             *pos += written;
             printExprRecursive(expr->as.assign.value, buffer, pos, capacity);
             if (*pos < capacity -1) buffer[(*pos)++] = ')'; else return;
             break;
        }
        case EXPR_BINARY: {
            // Determine operator string
            switch (expr->as.binary.operator.type) {
                case TOKEN_PLUS: opStr = "+"; break;
                case TOKEN_MINUS: opStr = "-"; break;
                case TOKEN_STAR: opStr = "*"; break;
                case TOKEN_SLASH: opStr = "/"; break;
                case TOKEN_EQUAL_EQUAL: opStr = "=="; break;
                case TOKEN_BANG_EQUAL: opStr = "!="; break;
                case TOKEN_GREATER: opStr = ">"; break;
                case TOKEN_GREATER_EQUAL: opStr = ">="; break;
                case TOKEN_LESS: opStr = "<"; break;
                case TOKEN_LESS_EQUAL: opStr = "<="; break;
                default: opStr = "opB?"; break;
            }
            written = snprintf(buffer + *pos, capacity - *pos, "(%s ", opStr);
             if (written < 0 || (size_t)written >= capacity - *pos) return;
            *pos += written;
            printExprRecursive(expr->as.binary.left, buffer, pos, capacity);
            if (*pos < capacity -1) buffer[(*pos)++] = ' '; else return;
            printExprRecursive(expr->as.binary.right, buffer, pos, capacity);
            if (*pos < capacity -1) buffer[(*pos)++] = ')'; else return;
            break;
        }
        case EXPR_GROUPING: {
            written = snprintf(buffer + *pos, capacity - *pos, "(group ");
             if (written < 0 || (size_t)written >= capacity - *pos) return;
            *pos += written;
            printExprRecursive(expr->as.grouping.expression, buffer, pos, capacity);
            if (*pos < capacity -1) buffer[(*pos)++] = ')'; else return;
            break;
        }
        case EXPR_LITERAL: {
            switch (expr->as.literal.type) {
                case TOKEN_NUMBER: written = snprintf(buffer + *pos, capacity - *pos, "%g", expr->as.literal.value.number); break;
                case TOKEN_STRING: written = snprintf(buffer + *pos, capacity - *pos, "\"%s\"", expr->as.literal.value.string); break;
                case TOKEN_TRUE:   written = snprintf(buffer + *pos, capacity - *pos, "true"); break;
                case TOKEN_FALSE:  written = snprintf(buffer + *pos, capacity - *pos, "false"); break;
                case TOKEN_NIL:    written = snprintf(buffer + *pos, capacity - *pos, "nil"); break;
                default:           written = snprintf(buffer + *pos, capacity - *pos, "?lit?"); break;
            }
             if (written < 0 || (size_t)written >= capacity - *pos) return;
             *pos += written;
            break;
        }
        case EXPR_UNARY: {
            opStr = (expr->as.unary.operator.type == TOKEN_MINUS) ? "-" : "!";
            written = snprintf(buffer + *pos, capacity - *pos, "(%s ", opStr);
             if (written < 0 || (size_t)written >= capacity - *pos) return;
            *pos += written;
            printExprRecursive(expr->as.unary.right, buffer, pos, capacity);
            if (*pos < capacity -1) buffer[(*pos)++] = ')'; else return;
            break;
        }
        case EXPR_VARIABLE: {
             Token name = expr->as.variable.name;
             written = snprintf(buffer + *pos, capacity - *pos, "%.*s", name.length, name.start);
             if (written < 0 || (size_t)written >= capacity - *pos) return;
             *pos += written;
            break;
        }

        default: 
             written = snprintf(buffer + *pos, capacity - *pos, "?Expr?");
             if (written < 0 || (size_t)written >= capacity - *pos) return;
             *pos += written;
             break;
    }
    // Ensure null termination within capacity
    buffer[*pos] = '\0';
}

char* printExpr(Expr* expr) {
    size_t capacity = 256;
    char* buffer = malloc(capacity);
    if (buffer == NULL) return NULL;

    size_t pos = 0;
    printExprRecursive(expr, buffer, &pos, capacity);


    return buffer;
} 