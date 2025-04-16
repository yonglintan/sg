#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "expr.h"

Expr* newBinary(Expr* left, Token operator, Expr* right) {
    Binary* expr = (Binary*)malloc(sizeof(Binary));
    expr->base.type = EXPR_BINARY;
    expr->left = left;
    expr->operator = operator;
    expr->right = right;
    return (Expr*)expr;
}

Expr* newGrouping(Expr* expression) {
    Grouping* expr = (Grouping*)malloc(sizeof(Grouping));
    expr->base.type = EXPR_GROUPING;
    expr->expression = expression;
    return (Expr*)expr;
}

Expr* newLiteralNumber(double value) {
    Literal* expr = (Literal*)malloc(sizeof(Literal));
    expr->base.type = EXPR_LITERAL;
    expr->literalType = LITERAL_NUMBER;
    expr->value.number = value;
    return (Expr*)expr;
}

Expr* newLiteralString(const char* value) {
    Literal* expr = (Literal*)malloc(sizeof(Literal));
    expr->base.type = EXPR_LITERAL;
    expr->literalType = LITERAL_STRING;
    expr->value.string = strdup(value);
    return (Expr*)expr;
}

Expr* newLiteralBool(bool value) {
    Literal* expr = (Literal*)malloc(sizeof(Literal));
    expr->base.type = EXPR_LITERAL;
    expr->literalType = LITERAL_BOOL;
    expr->value.boolean = value;
    return (Expr*)expr;
}

Expr* newLiteralNil() {
    Literal* expr = (Literal*)malloc(sizeof(Literal));
    expr->base.type = EXPR_LITERAL;
    expr->literalType = LITERAL_NIL;
    return (Expr*)expr;
}

Expr* newUnary(Token operator, Expr* right) {
    Unary* expr = (Unary*)malloc(sizeof(Unary));
    expr->base.type = EXPR_UNARY;
    expr->operator = operator;
    expr->right = right;
    return (Expr*)expr;
}

// Free function
void freeExpr(Expr* expr) {
    if (expr == NULL) return;
    
    switch (expr->type) {
        case EXPR_BINARY: {
            Binary* binary = (Binary*)expr;
            freeExpr(binary->left);
            freeExpr(binary->right);
            break;
        }
        case EXPR_GROUPING: {
            Grouping* grouping = (Grouping*)expr;
            freeExpr(grouping->expression);
            break;
        }
        case EXPR_LITERAL: {
            Literal* literal = (Literal*)expr;
            if (literal->literalType == LITERAL_STRING) {
                free(literal->value.string);
            }
            break;
        }
        case EXPR_UNARY: {
            Unary* unary = (Unary*)expr;
            freeExpr(unary->right);
            break;
        }
    }
    
    free(expr);
}

// Helper function to manage the string buffer for printing
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
    
    // Make sure we have enough space
    if (sb->capacity < sb->length + len + 1) {
        int newCapacity = (sb->capacity == 0) ? 64 : sb->capacity * 2;
        while (newCapacity < sb->length + len + 1) {
            newCapacity *= 2;
        }
        
        sb->buffer = (char*)realloc(sb->buffer, newCapacity);
        sb->capacity = newCapacity;
    }
    
    // Append the string
    strcpy(sb->buffer + sb->length, str);
    sb->length += len;
}

void freeStringBuilder(StringBuilder* sb) {
    free(sb->buffer);
    free(sb);
}

// Forward declaration of helper function
void printExprInternal(StringBuilder* sb, Expr* expr);

// Helper for parenthesizing expressions
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

// The main internal printing function
void printExprInternal(StringBuilder* sb, Expr* expr) {
    switch (expr->type) {
        case EXPR_BINARY: {
            Binary* binary = (Binary*)expr;
            char op[2] = {0};
            op[0] = *binary->operator.start;
            parenthesize(sb, op, 2, binary->left, binary->right);
            break;
        }
        case EXPR_GROUPING: {
            Grouping* grouping = (Grouping*)expr;
            parenthesize(sb, "group", 1, grouping->expression);
            break;
        }
        case EXPR_LITERAL: {
            Literal* literal = (Literal*)expr;
            switch (literal->literalType) {
                case LITERAL_NIL:
                    appendString(sb, "nil");
                    break;
                case LITERAL_BOOL:
                    appendString(sb, literal->value.boolean ? "true" : "false");
                    break;
                case LITERAL_NUMBER: {
                    char buffer[32];
                    snprintf(buffer, sizeof(buffer), "%g", literal->value.number);
                    appendString(sb, buffer);
                    break;
                }
                case LITERAL_STRING:
                    appendString(sb, literal->value.string);
                    break;
            }
            break;
        }
        case EXPR_UNARY: {
            Unary* unary = (Unary*)expr;
            char op[2] = {0};
            op[0] = *unary->operator.start;
            parenthesize(sb, op, 1, unary->right);
            break;
        }
    }
}


char* printExpr(Expr* expr) {
    StringBuilder* sb = newStringBuilder();
    printExprInternal(sb, expr);
    
    // We need to return a copy that will outlive the StringBuilder
    char* result = strdup(sb->buffer);
    freeStringBuilder(sb);
    
    return result;
} 