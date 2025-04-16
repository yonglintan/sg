#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "interpreter.h"

// global runtime error flag
static bool runtimeError = false;
static RuntimeError lastError;

static Expr* evaluate(Expr* expr);
static bool isTruthy(Expr* expr);
static bool isEqual(Expr* left, Expr* right);
static void checkNumberOperand(Token operator, Expr* operand);
static void checkNumberOperands(Token operator, Expr* left, Expr* right);
static void reportRuntimeError(Token token, const char* message);

// return a copy of an expression value
static Expr* copyValue(Expr* expr) {
    if (expr == NULL) return NULL;
    
    switch (expr->type) {
        case EXPR_LITERAL:
            switch (expr->as.literal.type) {
                case TOKEN_NUMBER:
                    return newLiteralNumberExpr(expr->as.literal.value.number);
                case TOKEN_STRING:
                    return newLiteralStringExpr(expr->as.literal.value.string);
                case TOKEN_TRUE:
                case TOKEN_FALSE:
                    return newLiteralBooleanExpr(expr->as.literal.value.boolean);
                case TOKEN_NIL:
                    return newLiteralNilExpr();
                default:
                    return newLiteralNilExpr();
            }
        default:
            return newLiteralNilExpr();
    }
}

Expr* interpret(Expr* expression) {
    runtimeError = false;
    
    Expr* result = evaluate(expression);
    
    return result;
}

bool hadRuntimeError(void) {
    return runtimeError;
}

void resetRuntimeError(void) {
    runtimeError = false;
}

static Expr* evaluate(Expr* expr) {
    if (expr == NULL) return NULL;
    
    switch (expr->type) {
        case EXPR_LITERAL:
            return copyValue(expr);
            
        case EXPR_GROUPING:
            return evaluate(expr->as.grouping.expression);
            
        case EXPR_UNARY: {
            Expr* right = evaluate(expr->as.unary.right);
            if (right == NULL) return NULL; // propagate errors
            
            switch (expr->as.unary.operator.type) {
                case TOKEN_MINUS:
                    checkNumberOperand(expr->as.unary.operator, right);
                    if (runtimeError) {
                        freeExpr(right);
                        return NULL;
                    }
                    
                    double value = -right->as.literal.value.number;
                    freeExpr(right);
                    return newLiteralNumberExpr(value);
                
                case TOKEN_BANG: {
                    bool valueB = !isTruthy(right);
                    freeExpr(right);
                    return newLiteralBooleanExpr(valueB);
                }
                
                default:
                    freeExpr(right);
                    reportRuntimeError(expr->as.unary.operator, "Invalid unary operator.");
                    return NULL;
            }
        }
        
        case EXPR_BINARY: {
            Expr* left = evaluate(expr->as.binary.left);
            if (left == NULL) return NULL; // propagate errors
            
            Expr* right = evaluate(expr->as.binary.right);
            if (right == NULL) {
                freeExpr(left);
                return NULL;
            }
            
            Expr* result = NULL;
            
            switch (expr->as.binary.operator.type) {
                // arithmetic operators
                case TOKEN_MINUS:
                    checkNumberOperands(expr->as.binary.operator, left, right);
                    if (!runtimeError) {
                        result = newLiteralNumberExpr(
                            left->as.literal.value.number - right->as.literal.value.number);
                    }
                    break;
                    
                case TOKEN_PLUS:
                    // addition or string concatenation
                    if (left->as.literal.type == TOKEN_NUMBER && 
                        right->as.literal.type == TOKEN_NUMBER) {
                        result = newLiteralNumberExpr(
                            left->as.literal.value.number + right->as.literal.value.number);
                    } else if (left->as.literal.type == TOKEN_STRING && 
                               right->as.literal.type == TOKEN_STRING) {
                        // string concatenation
                        int len1 = strlen(left->as.literal.value.string);
                        int len2 = strlen(right->as.literal.value.string);
                        char* concat = malloc(len1 + len2 + 1);
                        
                        if (concat != NULL) {
                            strcpy(concat, left->as.literal.value.string);
                            strcat(concat, right->as.literal.value.string);
                            result = newLiteralStringExpr(concat);
                            free(concat);
                        }
                    } else {
                        reportRuntimeError(expr->as.binary.operator, 
                            "Operands must be two numbers or two strings.");
                    }
                    break;
                    
                case TOKEN_SLASH:
                    checkNumberOperands(expr->as.binary.operator, left, right);
                    if (!runtimeError) {
                        // check for division by zero
                        if (right->as.literal.value.number == 0) {
                            reportRuntimeError(expr->as.binary.operator, "Division by zero.");
                        } else {
                            result = newLiteralNumberExpr(
                                left->as.literal.value.number / right->as.literal.value.number);
                        }
                    }
                    break;
                    
                case TOKEN_STAR:
                    checkNumberOperands(expr->as.binary.operator, left, right);
                    if (!runtimeError) {
                        result = newLiteralNumberExpr(
                            left->as.literal.value.number * right->as.literal.value.number);
                    }
                    break;
                    
                // comparison operators
                case TOKEN_GREATER:
                    checkNumberOperands(expr->as.binary.operator, left, right);
                    if (!runtimeError) {
                        result = newLiteralBooleanExpr(
                            left->as.literal.value.number > right->as.literal.value.number);
                    }
                    break;
                    
                case TOKEN_GREATER_EQUAL:
                    checkNumberOperands(expr->as.binary.operator, left, right);
                    if (!runtimeError) {
                        result = newLiteralBooleanExpr(
                            left->as.literal.value.number >= right->as.literal.value.number);
                    }
                    break;
                    
                case TOKEN_LESS:
                    checkNumberOperands(expr->as.binary.operator, left, right);
                    if (!runtimeError) {
                        result = newLiteralBooleanExpr(
                            left->as.literal.value.number < right->as.literal.value.number);
                    }
                    break;
                    
                case TOKEN_LESS_EQUAL:
                    checkNumberOperands(expr->as.binary.operator, left, right);
                    if (!runtimeError) {
                        result = newLiteralBooleanExpr(
                            left->as.literal.value.number <= right->as.literal.value.number);
                    }
                    break;
                    
                // equality operators
                case TOKEN_EQUAL_EQUAL:
                    result = newLiteralBooleanExpr(isEqual(left, right));
                    break;
                    
                case TOKEN_BANG_EQUAL:
                    result = newLiteralBooleanExpr(!isEqual(left, right));
                    break;
                    
                default:
                    reportRuntimeError(expr->as.binary.operator, "Invalid binary operator.");
                    break;
            }
            
            freeExpr(left);
            freeExpr(right);
            
            return result;
        }
    }
    
    return NULL;
}

static bool isTruthy(Expr* expr) {
    if (expr == NULL) return false;
    
    if (expr->type != EXPR_LITERAL) return true;
    
    switch (expr->as.literal.type) {
        case TOKEN_NIL:
            return false;
        case TOKEN_FALSE:
            return false;
        case TOKEN_TRUE:
            return true;
        default:
            return true;
    }
}

static bool isEqual(Expr* left, Expr* right) {
    // nil is only equal to nil
    if (left->as.literal.type == TOKEN_NIL && right->as.literal.type == TOKEN_NIL) return true;
    if (left->as.literal.type == TOKEN_NIL || right->as.literal.type == TOKEN_NIL) return false;
    
    // compare values based on their type
    if (left->as.literal.type != right->as.literal.type) return false;
    
    switch (left->as.literal.type) {
        case TOKEN_NUMBER:
            return left->as.literal.value.number == right->as.literal.value.number;
        case TOKEN_STRING:
            return strcmp(left->as.literal.value.string, right->as.literal.value.string) == 0;
        case TOKEN_TRUE:
        case TOKEN_FALSE:
            return left->as.literal.value.boolean == right->as.literal.value.boolean;
        default:
            return false;
    }
}

static void checkNumberOperand(Token operator, Expr* operand) {
    if (operand->type == EXPR_LITERAL && operand->as.literal.type == TOKEN_NUMBER) return;
    reportRuntimeError(operator, "Operand must be a number.");
}

static void checkNumberOperands(Token operator, Expr* left, Expr* right) {
    if (left->type == EXPR_LITERAL && left->as.literal.type == TOKEN_NUMBER &&
        right->type == EXPR_LITERAL && right->as.literal.type == TOKEN_NUMBER) return;
    reportRuntimeError(operator, "Operands must be numbers.");
}

static void reportRuntimeError(Token token, const char* message) {
    runtimeError = true;
    lastError.token = token;
    lastError.message = message;
    
    fprintf(stderr, "[line %d] Runtime Error: %s\n", token.line, message);
} 