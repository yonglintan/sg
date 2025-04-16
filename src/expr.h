#ifndef expr_h
#define expr_h

#include "scanner.h"

typedef enum {
    EXPR_BINARY,
    EXPR_GROUPING,
    EXPR_LITERAL,
    EXPR_UNARY
} ExprType;

typedef struct Expr Expr;

typedef struct {
    Expr* left;
    Token operator;
    Expr* right;
} BinaryExpr;

typedef struct {
    Expr* expression;
} GroupingExpr;

typedef struct {
    TokenType type;
    union {
        double number;
        bool boolean;
        char* string;
    } value;
} LiteralExpr;

typedef struct {
    Token operator;
    Expr* right;
} UnaryExpr;

struct Expr {
    ExprType type;
    union {
        BinaryExpr binary;
        GroupingExpr grouping;
        LiteralExpr literal;
        UnaryExpr unary;
    } as;
};

// we can probably refactor this later?
Expr* newBinaryExpr(Expr* left, Token operator, Expr* right);
Expr* newGroupingExpr(Expr* expression);
Expr* newLiteralNumberExpr(double value);
Expr* newLiteralBooleanExpr(bool value);
Expr* newLiteralStringExpr(char* value);
Expr* newLiteralNilExpr();
Expr* newUnaryExpr(Token operator, Expr* right);


void freeExpr(Expr* expr);

// print an expression (for debugging)
char* printExpr(Expr* expr);

#endif 