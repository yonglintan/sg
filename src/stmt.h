#ifndef stmt_h
#define stmt_h

#include "expr.h"
#include "scanner.h"

typedef enum {
    STMT_EXPRESSION,
    STMT_IF,
    STMT_PRINT,
    STMT_WHILE,
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
    Expr* condition;
    Stmt* thenBranch;
    Stmt* elseBranch;
} IfStmt;

typedef struct {
    Expr* expression;
} PrintStmt;

typedef struct {
    Expr* condition;
    Stmt* body;
} WhileStmt;

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
        IfStmt ifStmt;
        PrintStmt print;
        WhileStmt whileStmt;
        VarStmt var;
        BlockStmt block;
    } as;
};

// create new statements
Stmt* newExpressionStmt(Expr* expression);
Stmt* newIfStmt(Expr* condition, Stmt* thenBranch, Stmt* elseBranch);
Stmt* newPrintStmt(Expr* expression);
Stmt* newWhileStmt(Expr* condition, Stmt* body);
Stmt* newVarStmt(Token name, Expr* initializer);
Stmt* newBlockStmt(StmtList* statements);

// create a new statement list node
StmtList* newStmtList(Stmt* stmt, StmtList* next);

// freeing stuff
void freeStmt(Stmt* stmt);
void freeStmtList(StmtList* list);

#endif
