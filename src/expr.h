#ifndef expr_h
#define expr_h

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "scanner.h"

typedef enum {
    EXPR_BINARY,
    EXPR_GROUPING,
    EXPR_LITERAL,
    EXPR_UNARY
} ExprType;

typedef struct Expr Expr;
typedef struct Binary Binary;
typedef struct Grouping Grouping;
typedef struct Literal Literal;
typedef struct Unary Unary;

typedef enum {
    LITERAL_NUMBER,
    LITERAL_STRING,
    LITERAL_BOOL,
    LITERAL_NIL
} LiteralType;

typedef union {
    double number;
    char* string;
    bool boolean;
} LiteralValue;

struct Expr {
    ExprType type;
};

// (e.g., a + b)
struct Binary {
    Expr base;
    Expr* left;
    Token operator;
    Expr* right;
};

struct Grouping {
    Expr base;
    Expr* expression;
};

struct Literal {
    Expr base;
    LiteralType literalType;
    LiteralValue value;
};

struct Unary {
    Expr base;
    Token operator;
    Expr* right;
};

Expr* newBinary(Expr* left, Token operator, Expr* right);
Expr* newGrouping(Expr* expression);
Expr* newLiteralNumber(double value);
Expr* newLiteralString(const char* value);
Expr* newLiteralBool(bool value);
Expr* newLiteralNil();
Expr* newUnary(Token operator, Expr* right);

void freeExpr(Expr* expr);

// AST Printer, to delete later, for debugging 
char* printExpr(Expr* expr);

#endif 