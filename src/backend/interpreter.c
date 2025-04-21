#include "interpreter.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ast/expr.h"
#include "../ast/stmt.h"
#include "../frontend/scanner.h"
#include "../runtime/memory.h"
#include "../runtime/object.h"
#include "environment.h"
#include <time.h>

// --- Global State ---
static Environment* globalEnvironment = NULL;
static Environment* currentEnvironment = NULL;
static bool runtimeErrorOccurred = false;
static bool had_return = false;
static Value return_value = NIL_VAL;

// ===== Forward Declarations for Static Helpers =====
static Value evaluateExpr(Expr* expr);
static void executeStmt(Stmt* stmt);
static void executeBlock(StmtList* statements, Environment* environment);
static bool isTruthy(Value value);
static void checkNumberOperand(Token* operatorToken, Value operand);
static void checkNumberOperands(Token* operatorToken, Value left, Value right);
static Value callFunction(ObjFunction* function, Value* arguments, int arg_count);
static Value visitCallExpr(Expr* expr);
static Value clockNative(struct Interpreter* interpreter, int arg_count, Value* args);

// --- Interpreter Initialization and Cleanup ---
void initInterpreter() {
    if (globalEnvironment == NULL) {
        globalEnvironment = newEnvironment();
        if (globalEnvironment == NULL) {
            fprintf(stderr, "Aiyo die already lah: Global environment jialat, cannot init!\n");
            exit(74);
        }

        ObjNative* clockFn = newNative(0, clockNative);
        if (clockFn != NULL) {
            clockFn->arity = 0;
            clockFn->function = clockNative;
            environmentDefine(globalEnvironment, "clock", OBJ_VAL(clockFn));
        }
    }
    currentEnvironment = globalEnvironment;
    runtimeErrorOccurred = false;
}

void freeInterpreter() {
    if (globalEnvironment != NULL) {
        freeEnvironment(globalEnvironment);
        globalEnvironment = NULL;
        currentEnvironment = NULL;
    }
}

// --- Runtime Error Handling ---

// Note: Uses the name `runtimeError` as defined in the header.
void runtimeError(Token* token, const char* format, ...) {
    if (runtimeErrorOccurred) return; // Report only the first error
    runtimeErrorOccurred = true;

    fprintf(stderr, "[line %d] Wah piang! Runtime problem here lah", token ? token->line : 0);
    fprintf(stderr, ": ");

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

bool hadRuntimeError() { return runtimeErrorOccurred; }

void resetRuntimeError() { runtimeErrorOccurred = false; }

// --- Statement Execution ---

// Main entry point for executing code
void interpretStatements(StmtList* statements) {
    StmtList* current = statements;
    while (current != NULL && !runtimeErrorOccurred) {
        executeStmt(current->stmt);
        current = current->next;
    }
}

// Executes a single statement
static void executeStmt(Stmt* stmt) {
    if (stmt == NULL || runtimeErrorOccurred) return;

    switch (stmt->type) {
        case STMT_EXPRESSION: {
            evaluateExpr(stmt->as.expression.expression); // Evaluate for side effects
            break;
        }
        case STMT_IF: {
            if (isTruthy(evaluateExpr(stmt->as.ifStmt.condition))) {
                executeStmt(stmt->as.ifStmt.thenBranch);
            } else if (stmt->as.ifStmt.elseBranch != NULL) {
                executeStmt(stmt->as.ifStmt.elseBranch);
            }
            break;
        }
        case STMT_PRINT: {
            Value value = evaluateExpr(stmt->as.print.expression);
            if (runtimeErrorOccurred) return;
            printValue(value);
            printf("\n");
            // TODO: Handle freeing potential heap objects from value (e.g.,
            // strings)
            break;
        }
        case STMT_WHILE: {
            while (isTruthy(evaluateExpr(stmt->as.whileStmt.condition))) {
                executeStmt(stmt->as.whileStmt.body);
            }
            break;
        }
        case STMT_VAR: {
            Value value = NIL_VAL; // Default value
            if (stmt->as.var.initializer != NULL) {
                value = evaluateExpr(stmt->as.var.initializer);
                if (runtimeErrorOccurred) return;
            }
            // Get a temporary C string for the variable name
            char* name = malloc(stmt->as.var.name.length + 1);
            if (name == NULL) {
                runtimeError(&stmt->as.var.name,
                             "Memory error processing variable name.");
                return;
            }
            strncpy(name, stmt->as.var.name.start, stmt->as.var.name.length);
            name[stmt->as.var.name.length] = '\0';

            if (!environmentDefine(currentEnvironment, name, value)) {
                runtimeError(&stmt->as.var.name,
                             "Memory error defining variable '%s'.", name);
            }
            free(name);
            break;
        }
        case STMT_BLOCK: {
            // creates a new environment for the block, enclosing the current one
            Environment* blockEnvironment = newEnclosedEnvironment(currentEnvironment);
            if (blockEnvironment == NULL) {
                runtimeError(NULL, "Memory error creating block environment.");
                return;
            }
            // executes the block's statements in the new environment
            executeBlock(stmt->as.block.statements, blockEnvironment);
            // environment is freed within executeBlock after execution
            break;
        }
        case STMT_FUNCTION: {
            // create function object with the current environment as closure
            ObjFunction* function = newFunction(stmt, currentEnvironment);
            if (function == NULL) {
                runtimeError(&stmt->as.function.name, "Memory error creating function.");
                return;
            }

            char* name = malloc(stmt->as.function.name.length + 1);
            if (name == NULL) {
                runtimeError(&stmt->as.function.name, "Memory error processing function name.");
                return;
            }
            strncpy(name, stmt->as.function.name.start, stmt->as.function.name.length);
            name[stmt->as.function.name.length] = '\0';

            environmentDefine(currentEnvironment, name, OBJ_VAL(function));
            free(name);
            break;
        }
        case STMT_RETURN: {
            Value value = NIL_VAL;
            if (stmt->as.return_stmt.value != NULL) {
                value = evaluateExpr(stmt->as.return_stmt.value);
                if (runtimeErrorOccurred) return;
            }

            had_return = true;
            return_value = value;
            break;
        }
        default:
            runtimeError(NULL, "Interpreter error: Unknown statement type %d.",
                         stmt->type);
            break;
    }
}

static void executeBlock(StmtList* statements, Environment* environment) {
    Environment* previousEnvironment = currentEnvironment;
    // sets the new environment as current
    currentEnvironment = environment;

    StmtList* current = statements;
    // executes statements until the end or a runtime error occurs or if a return occurs
    while (current != NULL && !runtimeErrorOccurred && !had_return) {
        executeStmt(current->stmt);
        current = current->next;
    }

    currentEnvironment = previousEnvironment;
    freeEnvironment(environment);
}

static Value callFunction(ObjFunction* function, Value* arguments, int arg_count) {
    (void)arg_count;
    Environment* environment = newEnclosedEnvironment(function->closure);
    if (environment == NULL) {
        runtimeError(NULL, "Memory error creating function environment.");
        return NIL_VAL;
    }

    // Bind arguments to parameters
    for (int i = 0; i < function->declaration->as.function.param_count; i++) {
        Token param = function->declaration->as.function.params[i];

        char* name = malloc(param.length + 1);
        if (name == NULL) {
            runtimeError(&param, "Memory error processing parameter name.");
            freeEnvironment(environment);
            return NIL_VAL;
        }
        strncpy(name, param.start, param.length);
        name[param.length] = '\0';

        environmentDefine(environment, name, arguments[i]);
        free(name);
    }

    Environment* previous = currentEnvironment;
    currentEnvironment = environment;

    had_return = false;

    executeBlock(function->declaration->as.function.body, environment);

    currentEnvironment = previous;

    if (had_return) {
        Value result = return_value;
        had_return = false;
        return result;
    }

    return NIL_VAL;
}

// ===== Expression Evaluation stuff =====
static bool isTruthy(Value value) {
    if (IS_NIL(value)) return false;
    if (IS_BOOL(value)) return AS_BOOL(value);
    return true; // Numbers (and future objects) are truthy
}

static void checkNumberOperand(Token* operatorToken, Value operand) {
    if (IS_NUMBER(operand)) return;
    runtimeError(operatorToken, "Operand must be a number.");
}

static void checkNumberOperands(Token* operatorToken, Value left, Value right) {
    if (IS_NUMBER(left) && IS_NUMBER(right)) return;
    runtimeError(operatorToken, "Operands must be numbers.");
}

static Value clockNative(struct Interpreter* interpreter, int arg_count, Value* args) {
    (void)interpreter;
    (void)arg_count;
    (void)args;
    return NUMBER_VAL((double)time(NULL));
}

static Value evaluateExpr(Expr* expr) {
    if (expr == NULL || runtimeErrorOccurred) return NIL_VAL;

    switch (expr->type) {
        case EXPR_LITERAL: {
            switch (expr->as.literal.type) {
                case TOKEN_NUMBER:
                    return NUMBER_VAL(expr->as.literal.value.number);
                case TOKEN_STRING: {
                    int length = strlen(expr->as.literal.value.string);
                    ObjString* stringObj = copyString(expr->as.literal.value.string, length);
                    if (stringObj == NULL) {
                        runtimeError(NULL,
                                     "Memory error creating string value.");
                        return NIL_VAL;
                    }
                    return OBJ_VAL(stringObj);
                }
                case TOKEN_CORRECT:
                    return BOOL_VAL(true);
                case TOKEN_WRONG:
                    return BOOL_VAL(false);
                case TOKEN_NIL:
                    return NIL_VAL;
                default:
                    runtimeError(NULL,
                                 "Interpreter error: Unknown literal token.");
                    return NIL_VAL;
            }
        }
        case EXPR_LOGICAL: {
            Value left = evaluateExpr(expr->as.logical.left);
            if (runtimeErrorOccurred) return NIL_VAL;
            switch (expr->as.logical.oper.type) {
                case TOKEN_OR:
                    if (isTruthy(left)) return left;
                    break;
                case TOKEN_AND:
                    if (!isTruthy(left)) return left;
                    break;
                default:
                    runtimeError(&expr->as.logical.oper, "Interpreter error: Unknown logical op.");
                    return NIL_VAL;
            }
            return evaluateExpr(expr->as.logical.right);
        }
        case EXPR_GROUPING: {
            return evaluateExpr(expr->as.grouping.expression);
        }
        case EXPR_UNARY: {
            Value right = evaluateExpr(expr->as.unary.right);
            if (runtimeErrorOccurred) return NIL_VAL;
            switch (expr->as.unary.oper.type) {
                case TOKEN_BANG:
                    return BOOL_VAL(!isTruthy(right));
                case TOKEN_MINUS:
                    checkNumberOperand(&expr->as.unary.oper, right);
                    if (runtimeErrorOccurred) return NIL_VAL;
                    return NUMBER_VAL(-AS_NUMBER(right));
                default:
                    runtimeError(&expr->as.unary.oper,
                                 "Interpreter error: Unknown unary op.");
                    return NIL_VAL;
            }
        }
        case EXPR_BINARY: {
            Value left = evaluateExpr(expr->as.binary.left);
            if (runtimeErrorOccurred) return NIL_VAL;
            Value right = evaluateExpr(expr->as.binary.right);
            if (runtimeErrorOccurred) return NIL_VAL;
            switch (expr->as.binary.oper.type) {
                case TOKEN_GREATER:
                    checkNumberOperands(&expr->as.binary.oper, left, right);
                    if (runtimeErrorOccurred) return NIL_VAL;
                    return BOOL_VAL(AS_NUMBER(left) > AS_NUMBER(right));
                case TOKEN_GREATER_EQUAL:
                    checkNumberOperands(&expr->as.binary.oper, left, right);
                    if (runtimeErrorOccurred) return NIL_VAL;
                    return BOOL_VAL(AS_NUMBER(left) >= AS_NUMBER(right));
                case TOKEN_LESS:
                    checkNumberOperands(&expr->as.binary.oper, left, right);
                    if (runtimeErrorOccurred) return NIL_VAL;
                    return BOOL_VAL(AS_NUMBER(left) < AS_NUMBER(right));
                case TOKEN_LESS_EQUAL:
                    checkNumberOperands(&expr->as.binary.oper, left, right);
                    if (runtimeErrorOccurred) return NIL_VAL;
                    return BOOL_VAL(AS_NUMBER(left) <= AS_NUMBER(right));
                case TOKEN_BANG_EQUAL:
                    return BOOL_VAL(!valuesEqual(left, right));
                case TOKEN_EQUAL_EQUAL:
                    return BOOL_VAL(valuesEqual(left, right));
                case TOKEN_MINUS:
                    checkNumberOperands(&expr->as.binary.oper, left, right);
                    if (runtimeErrorOccurred) return NIL_VAL;
                    return NUMBER_VAL(AS_NUMBER(left) - AS_NUMBER(right));
                case TOKEN_STAR:
                    checkNumberOperands(&expr->as.binary.oper, left, right);
                    if (runtimeErrorOccurred) return NIL_VAL;
                    return NUMBER_VAL(AS_NUMBER(left) * AS_NUMBER(right));
                case TOKEN_SLASH:
                    checkNumberOperands(&expr->as.binary.oper, left, right);
                    if (runtimeErrorOccurred) return NIL_VAL;
                    if (AS_NUMBER(right) == 0) {
                        runtimeError(&expr->as.binary.oper,
                                     "Division by zero.");
                        return NIL_VAL;
                    }
                    return NUMBER_VAL(AS_NUMBER(left) / AS_NUMBER(right));
                case TOKEN_PLUS:
                    if (IS_NUMBER(left) && IS_NUMBER(right)) {
                        return NUMBER_VAL(AS_NUMBER(left) + AS_NUMBER(right));
                    }

                    // string concatenation
                    if (IS_STRING(left) && IS_STRING(right)) {
                        int leftLength = AS_STRING(left)->length;
                        int rightLength = AS_STRING(right)->length;
                        int totalLength = leftLength + rightLength;
                        char* result = ALLOCATE(char, totalLength + 1);
                        if (result == NULL) {
                            runtimeError(
                                NULL, "Memory error creating string result.");
                            return NIL_VAL;
                        }
                        strncpy(result, AS_STRING(left)->chars, leftLength);
                        strncpy(result + leftLength, AS_STRING(right)->chars,
                                rightLength);
                        result[totalLength] = '\0';
                        return OBJ_VAL(copyString(result, totalLength));
                    }
                    runtimeError(
                        &expr->as.binary.oper,
                        "Operands must be two numbers or two strings.");
                    return NIL_VAL;
                default:
                    runtimeError(&expr->as.binary.oper,
                                 "Interpreter error: Unknown binary op.");
                    return NIL_VAL;
            }
        }
        case EXPR_VARIABLE: {
            Value value;
            if (environmentGet(currentEnvironment, &expr->as.variable.name,
                               &value)) {
                return value;
            } else {
                char* name = malloc(expr->as.variable.name.length + 1);
                if (name) {
                    strncpy(name, expr->as.variable.name.start,
                            expr->as.variable.name.length);
                    name[expr->as.variable.name.length] = '\0';
                    runtimeError(&expr->as.variable.name,
                                 "Undefined variable '%s'.", name);
                    free(name);
                } else {
                    runtimeError(&expr->as.variable.name,
                                 "Undefined variable (mem err).");
                }
                return NIL_VAL;
            }
        }
        case EXPR_ASSIGN: {
            Value value = evaluateExpr(expr->as.assign.value);
            if (runtimeErrorOccurred) return NIL_VAL;
            if (environmentAssign(currentEnvironment, &expr->as.assign.name,
                                  value)) {
                return value;
            } else {
                char* name = malloc(expr->as.assign.name.length + 1);
                if (name) {
                    strncpy(name, expr->as.assign.name.start,
                            expr->as.assign.name.length);
                    name[expr->as.assign.name.length] = '\0';
                    runtimeError(&expr->as.assign.name,
                                 "Undefined variable '%s' for assignment.",
                                 name);
                    free(name);
                } else {
                    runtimeError(
                        &expr->as.assign.name,
                        "Undefined variable for assignment (mem err).");
                }
                return NIL_VAL;
            }
        }
        case EXPR_CALL:
            return visitCallExpr(expr);

        default:
            runtimeError(NULL, "Interpreter error: Unknown expression type %d.",
                         expr->type);
            return NIL_VAL;
    }
    runtimeError(NULL, "Interpreter error: Unknown expression type %d.",
                 expr->type);
    return NIL_VAL;
}

static Value visitCallExpr(Expr* expr) {
    Value callee = evaluateExpr(expr->as.call.callee);
    if (runtimeErrorOccurred) return NIL_VAL;

    Value* arguments = malloc(sizeof(Value) * expr->as.call.arg_count);
    if (arguments == NULL && expr->as.call.arg_count > 0) {
        runtimeError(&expr->as.call.paren, "Memory error evaluating function arguments.");
        return NIL_VAL;
    }

    for (int i = 0; i < expr->as.call.arg_count; i++) {
        arguments[i] = evaluateExpr(expr->as.call.arguments[i]);
        if (runtimeErrorOccurred) {
            free(arguments);
            return NIL_VAL;
        }
    }

    if (IS_OBJ(callee) && OBJ_TYPE(callee) == OBJ_FUNCTION) {
        ObjFunction* function = (ObjFunction*)AS_OBJ(callee);

        if (expr->as.call.arg_count != function->arity) {
            char error[100];
            sprintf(error, "Eh hello, suppose to get %d argument(s) but you give %d only leh.",
                    function->arity, expr->as.call.arg_count);
            runtimeError(&expr->as.call.paren, error);
            free(arguments);
            return NIL_VAL;
        }

        Value result = callFunction(function, arguments, expr->as.call.arg_count);
        free(arguments);
        return result;
    } else if (IS_OBJ(callee) && OBJ_TYPE(callee) == OBJ_NATIVE) {
        ObjNative* native = (ObjNative*)AS_OBJ(callee);

        if (expr->as.call.arg_count != native->arity) {
            char error[100];
            sprintf(error, "Eh hello, suppose to get %d argument(s) but you give %d only leh.",
                    native->arity, expr->as.call.arg_count);
            runtimeError(&expr->as.call.paren, error);
            free(arguments);
            return NIL_VAL;
        }

        Value result = native->function(NULL, expr->as.call.arg_count, arguments);
        free(arguments);
        return result;
    } else {
        runtimeError(&expr->as.call.paren, "Can only call functions.");
        free(arguments);
        return NIL_VAL;
    }
}
