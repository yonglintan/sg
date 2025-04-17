#ifndef stmt_h
#define stmt_h

#include "expr.h"
#include "scanner.h"

typedef enum {
    STMT_EXPRESSION,
    STMT_PRINT,
    STMT_VAR,
    STMT_BLOCK
} StmtType;


typedef struct Stmt Stmt;
typedef struct StmtList StmtList;

// statement list, this is a linked list of statements
struct StmtList {
    Stmt* stmt;
    StmtList* next;
};

typedef struct {
    Expr* expression;
} ExpressionStmt;

typedef struct {
    Expr* expression;
} PrintStmt;

typedef struct {
    Token name;
    Expr* initializer;
} VarStmt;

typedef struct {
    StmtList* statements;
} BlockStmt;

struct Stmt {
    StmtType type;
    union {
        ExpressionStmt expression;
        PrintStmt print;
        VarStmt var;
        BlockStmt block;
    } as;
};

// create new statements
Stmt* newExpressionStmt(Expr* expression);
Stmt* newPrintStmt(Expr* expression);
Stmt* newVarStmt(Token name, Expr* initializer);
Stmt* newBlockStmt(StmtList* statements);

// create a new statement list node
StmtList* newStmtList(Stmt* stmt, StmtList* next);

// freeing stuff
void freeStmt(Stmt* stmt);
void freeStmtList(StmtList* list);

#endif 