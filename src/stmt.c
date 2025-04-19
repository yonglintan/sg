#include "stmt.h"
#include "expr.h"
#include <stdlib.h>

// creation of statements. i think we can probably refactor this but idk if keeping it separate for now is better in case
// we gta do more specific stuff
Stmt* newExpressionStmt(Expr* expression) {
    Stmt* stmt = (Stmt*)malloc(sizeof(Stmt));
    stmt->type = STMT_EXPRESSION;
    stmt->as.expression.expression = expression;
    return stmt;
}

Stmt* newIfStmt(Expr* condition, Stmt* thenBranch, Stmt* elseBranch) {
    Stmt* stmt = (Stmt*)malloc(sizeof(Stmt));
    stmt->type = STMT_IF;
    stmt->as.ifStmt.condition = condition;
    stmt->as.ifStmt.thenBranch = thenBranch;
    stmt->as.ifStmt.elseBranch = elseBranch;
    return stmt;
}

Stmt* newPrintStmt(Expr* expression) {
    Stmt* stmt = (Stmt*)malloc(sizeof(Stmt));
    stmt->type = STMT_PRINT;
    stmt->as.print.expression = expression;
    return stmt;
}

Stmt* newWhileStmt(Expr* condition, Stmt* body) {
    Stmt* stmt = (Stmt*)malloc(sizeof(Stmt));
    stmt->type = STMT_WHILE;
    stmt->as.whileStmt.condition = condition;
    stmt->as.whileStmt.body = body;
    return stmt;
}

Stmt* newVarStmt(Token name, Expr* initializer) {
    Stmt* stmt = (Stmt*)malloc(sizeof(Stmt));
    stmt->type = STMT_VAR;
    stmt->as.var.name = name;
    stmt->as.var.initializer = initializer;
    return stmt;
}

Stmt* newBlockStmt(StmtList* statements) {
    Stmt* stmt = (Stmt*)malloc(sizeof(Stmt));
    stmt->type = STMT_BLOCK;
    stmt->as.block.statements = statements;
    return stmt;
}

StmtList* newStmtList(Stmt* stmt, StmtList* next) {
    StmtList* list = (StmtList*)malloc(sizeof(StmtList));
    list->stmt = stmt;
    list->next = next;
    return list;
}

Stmt* newFunctionStmt(Token name, int param_count, Token* params, StmtList* body) {
    Stmt* stmt = (Stmt*)malloc(sizeof(Stmt));
    if (stmt == NULL) return NULL;
    stmt->type = STMT_FUNCTION;
    stmt->as.function.name = name;
    stmt->as.function.param_count = param_count;
    stmt->as.function.params = params;
    stmt->as.function.body = body;
    return stmt;
}

Stmt* newReturnStmt(Token keyword, Expr* value) {
    Stmt* stmt = (Stmt*)malloc(sizeof(Stmt));
    if (stmt == NULL) return NULL;
    stmt->type = STMT_RETURN;
    stmt->as.return_stmt.keyword = keyword;
    stmt->as.return_stmt.value = value;
    return stmt;
}

// free a statement (and any expressions it contains)
void freeStmt(Stmt* stmt) {
    if (stmt == NULL) return;

    switch (stmt->type) {
        case STMT_EXPRESSION:
            freeExpr(stmt->as.expression.expression);
            break;
        case STMT_IF:
            freeExpr(stmt->as.ifStmt.condition);
            freeStmt(stmt->as.ifStmt.thenBranch);
            freeStmt(stmt->as.ifStmt.elseBranch);
            break;
        case STMT_PRINT:
            freeExpr(stmt->as.print.expression);
            break;
        case STMT_VAR:
            if (stmt->as.var.initializer != NULL) {
                freeExpr(stmt->as.var.initializer);
            }
            break;
        case STMT_BLOCK:
            freeStmtList(stmt->as.block.statements);
            break;
        case STMT_FUNCTION:
            free(stmt->as.function.params);
            freeStmtList(stmt->as.function.body);
            break;
        case STMT_RETURN:
            if (stmt->as.return_stmt.value != NULL) {
                freeExpr(stmt->as.return_stmt.value);
            }
            break;
    }

    free(stmt);
}

// free a statement list
void freeStmtList(StmtList* list) {
    if (list == NULL) return;

    // recursing through and freeing sub-statements first
    freeStmtList(list->next);

    // then we free the current and list node
    freeStmt(list->stmt);
    free(list);
}
