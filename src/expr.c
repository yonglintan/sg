#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "expr.h"

Expr* newBinaryExpr(Expr* left, Token operator, Expr* right) {
    Expr* expr = (Expr*)malloc(sizeof(Expr));
    expr->type = EXPR_BINARY;
    expr->as.binary.left = left;
    expr->as.binary.operator = operator;
    expr->as.binary.right = right;
    return expr;
}

Expr* newGroupingExpr(Expr* expression) {
    Expr* expr = (Expr*)malloc(sizeof(Expr));
    expr->type = EXPR_GROUPING;
    expr->as.grouping.expression = expression;
    return expr;
}

Expr* newLiteralNumberExpr(double value) {
    Expr* expr = (Expr*)malloc(sizeof(Expr));
    expr->type = EXPR_LITERAL;
    expr->as.literal.type = TOKEN_NUMBER;
    expr->as.literal.value.number = value;
    return expr;
}

Expr* newLiteralBooleanExpr(bool value) {
    Expr* expr = (Expr*)malloc(sizeof(Expr));
    expr->type = EXPR_LITERAL;
    expr->as.literal.type = value ? TOKEN_TRUE : TOKEN_FALSE;
    expr->as.literal.value.boolean = value;
    return expr;
}

Expr* newLiteralStringExpr(char* value) {
    Expr* expr = (Expr*)malloc(sizeof(Expr));
    expr->type = EXPR_LITERAL;
    expr->as.literal.type = TOKEN_STRING;
    expr->as.literal.value.string = strdup(value);
    return expr;
}

Expr* newLiteralNilExpr() {
    Expr* expr = (Expr*)malloc(sizeof(Expr));
    expr->type = EXPR_LITERAL;
    expr->as.literal.type = TOKEN_NIL;
    return expr;
}

Expr* newUnaryExpr(Token operator, Expr* right) {
    Expr* expr = (Expr*)malloc(sizeof(Expr));
    expr->type = EXPR_UNARY;
    expr->as.unary.operator = operator;
    expr->as.unary.right = right;
    return expr;
}

void freeExpr(Expr* expr) {
    if (expr == NULL) return;
    
    switch (expr->type) {
        case EXPR_BINARY:
            freeExpr(expr->as.binary.left);
            freeExpr(expr->as.binary.right);
            break;
        case EXPR_GROUPING:
            freeExpr(expr->as.grouping.expression);
            break;
        case EXPR_LITERAL:
            if (expr->as.literal.type == TOKEN_STRING) {
                free(expr->as.literal.value.string);
            }
            break;
        case EXPR_UNARY:
            freeExpr(expr->as.unary.right);
            break;
    }
    
    free(expr);
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

char* printExpr(Expr* expr) {
    StringBuilder* sb = newStringBuilder();
    printExprInternal(sb, expr);

    // return a version that will outlive the stringbuilder
    char* result = strdup(sb->buffer);
    freeStringBuilder(sb);
    
    return result;
} 