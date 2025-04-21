#include "resolver.h"
#include "../backend/environment.h"
#include "../backend/interpreter.h"
#include "../runtime/object.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char* name;
    bool defined;
} ScopeEntry;

typedef struct Scope {
    ScopeEntry* entries;
    int count;
    int capacity;
    struct Scope* next;
} Scope;

static Scope* scopeStack = NULL;

static void beginScope() {
    Scope* scope = (Scope*)malloc(sizeof(Scope));
    scope->count = 0;
    scope->capacity = 8;
    scope->entries = (ScopeEntry*)malloc(sizeof(ScopeEntry) * scope->capacity);
    scope->next = scopeStack;
    scopeStack = scope;
}

static void endScope() {
    Scope* top = scopeStack;
    for (int i = 0; i < top->count; i++) {
    }
    free(top->entries);
    scopeStack = top->next;
    free(top);
}

static void declare(Token name) {
    if (scopeStack == NULL) return;
    for (int i = 0; i < scopeStack->count; i++) {
        if (strncmp(scopeStack->entries[i].name, name.start, name.length) == 0 && strlen(scopeStack->entries[i].name) == name.length) {
            fprintf(stderr, "[line %d] Aiyo problem sia: This variable already declare in this scope liao.\n", name.line);
            return;
        }
    }
    if (scopeStack->count >= scopeStack->capacity) {
        scopeStack->capacity *= 2;
        scopeStack->entries = realloc(scopeStack->entries, sizeof(ScopeEntry) * scopeStack->capacity);
    }
    char* nameCopy = malloc(name.length + 1);
    if (nameCopy == NULL) {
        fprintf(stderr, "Memory problem lah: Cannot allocate space for name copy sia.\n");
        exit(1);
    }
    memcpy(nameCopy, name.start, name.length);
    nameCopy[name.length] = '\0';

    scopeStack->entries[scopeStack->count++] = (ScopeEntry) { nameCopy, false };
}

static void define(Token name) {
    if (scopeStack == NULL) return;
    for (int i = 0; i < scopeStack->count; i++) {
        if (strncmp(scopeStack->entries[i].name, name.start, name.length) == 0 && strlen(scopeStack->entries[i].name) == name.length) {
            scopeStack->entries[i].defined = true;
            return;
        }
    }
}

static void resolveStmt(Interpreter* interpreter, Stmt* stmt);
static void resolveExpr(Interpreter* interpreter, Expr* expr);

static void resolveFunction(Interpreter* interpreter, Stmt* function) {
    declare(function->as.function.name);
    define(function->as.function.name);
    beginScope();
    for (int i = 0; i < function->as.function.param_count; i++) {
        declare(function->as.function.params[i]);
        define(function->as.function.params[i]);
    }
    StmtList* body = function->as.function.body;
    while (body != NULL) {
        resolveStmt(interpreter, body->stmt);
        body = body->next;
    }
    endScope();
}

static void resolveStmt(Interpreter* interpreter, Stmt* stmt) {
    if (stmt == NULL) return; // defensive check
    switch (stmt->type) {
        case STMT_BLOCK:
            beginScope();
            for (StmtList* list = stmt->as.block.statements; list != NULL; list = list->next) {
                resolveStmt(interpreter, list->stmt);
            }
            endScope();
            break;
        case STMT_VAR:
            printf("Resolving var: %.*s\n", stmt->as.var.name.length, stmt->as.var.name.start);
            declare(stmt->as.var.name);
            if (stmt->as.var.initializer != NULL) {
                resolveExpr(interpreter, stmt->as.var.initializer);
            }
            define(stmt->as.var.name);
            break;
        case STMT_FUNCTION:
            resolveFunction(interpreter, stmt);
            break;
        case STMT_EXPRESSION:
            resolveExpr(interpreter, stmt->as.expression.expression);
            break;
        case STMT_PRINT:
            resolveExpr(interpreter, stmt->as.print.expression);
            break;
        case STMT_IF:
            resolveExpr(interpreter, stmt->as.ifStmt.condition);
            resolveStmt(interpreter, stmt->as.ifStmt.thenBranch);
            if (stmt->as.ifStmt.elseBranch != NULL)
                resolveStmt(interpreter, stmt->as.ifStmt.elseBranch);
            break;
        case STMT_WHILE:
            resolveExpr(interpreter, stmt->as.whileStmt.condition);
            resolveStmt(interpreter, stmt->as.whileStmt.body);
            break;
        case STMT_RETURN:
            if (stmt->as.return_stmt.value != NULL) {
                resolveExpr(interpreter, stmt->as.return_stmt.value);
            }
            break;
    }
}

static void resolveExpr(Interpreter* interpreter, Expr* expr) {
    if (expr == NULL) return;
    switch (expr->type) {
        case EXPR_ASSIGN:
            resolveExpr(interpreter, expr->as.assign.value);
            break;
        case EXPR_LOGICAL:
            resolveExpr(interpreter, expr->as.logical.left);
            resolveExpr(interpreter, expr->as.logical.right);
            break;
        case EXPR_BINARY:
            resolveExpr(interpreter, expr->as.binary.left);
            resolveExpr(interpreter, expr->as.binary.right);
            break;
        case EXPR_UNARY:
            resolveExpr(interpreter, expr->as.unary.right);
            break;
        case EXPR_GROUPING:
            resolveExpr(interpreter, expr->as.grouping.expression);
            break;
        case EXPR_CALL:
            resolveExpr(interpreter, expr->as.call.callee);
            for (int i = 0; i < expr->as.call.arg_count; i++) {
                resolveExpr(interpreter, expr->as.call.arguments[i]);
            }
            break;
        case EXPR_VARIABLE: {
            for (Scope* scope = scopeStack; scope != NULL; scope = scope->next) {
                for (int i = 0; i < scope->count; i++) {
                    if (strncmp(scope->entries[i].name, expr->as.variable.name.start, expr->as.variable.name.length) == 0 && strlen(scope->entries[i].name) == expr->as.variable.name.length) {
                        if (!scope->entries[i].defined) {
                            fprintf(stderr, "[line %d] Aiyo problem sia: How to read local variable when initializing itself?\n", expr->as.variable.name.line);
                        }
                        return;
                    }
                }
            }
            break;
        }
        case EXPR_LITERAL:
            break;
    }
}

void resolve(Interpreter* interpreter, StmtList* statements) {
    for (StmtList* current = statements; current != NULL; current = current->next) {
        resolveStmt(interpreter, current->stmt);
    }
}
