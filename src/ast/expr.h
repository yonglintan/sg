#ifndef expr_h
#define expr_h

#include "../frontend/scanner.h"
#include <stdbool.h> // Include for bool type used in LiteralExpr

typedef enum {
    EXPR_ASSIGN, // New: Assignment like x = 1
    EXPR_LOGICAL,
    EXPR_BINARY,
    EXPR_GROUPING,
    EXPR_LITERAL,
    EXPR_UNARY,
    EXPR_VARIABLE, // New: Variable access like x
    EXPR_CALL // New: Function call expression
} ExprType;

// Forward declaration needed for nested expressions
typedef struct Expr Expr;

// --- Expression Struct Definitions ---

// Assignment: identifier = value
typedef struct {
    Token name; // The variable token (identifier)
    Expr* value; // The expression being assigned
} AssignExpr;

// Binary: left op right
typedef struct {
    Expr* left;
    Token oper;
    Expr* right;
} LogicalExpr;

// Binary: left op right
typedef struct {
    Expr* left;
    Token oper;
    Expr* right;
} BinaryExpr;

// Call:
typedef struct {
    Expr* callee;
    Token paren; // closing parenthesis for error reporting
    int arg_count;
    Expr** arguments;
} CallExpr;

// Grouping: ( expression )
typedef struct {
    Expr* expression;
} GroupingExpr;

// Literal: number, string, true, false, nil
typedef struct {
    // Store the literal value directly in the expression node
    // We can use the TokenType to know which field of the union is valid
    TokenType type; // TOKEN_NUMBER, TOKEN_STRING, TOKEN_TRUE, etc.
    union {
        double number;
        bool boolean;
        char* string; // NOTE: Strings would typically be heap objects (later)
    } value;
} LiteralExpr;

// Unary: op right
typedef struct {
    Token oper;
    Expr* right;
} UnaryExpr;

// Variable: identifier
typedef struct {
    Token name; // The variable token (identifier)
} VariableExpr;

// --- Main Expression Struct (using a tagged union) ---
struct Expr {
    ExprType type;
    union {
        AssignExpr assign; // New
        LogicalExpr logical;
        BinaryExpr binary;
        CallExpr call;
        GroupingExpr grouping;
        LiteralExpr literal;
        UnaryExpr unary;
        VariableExpr variable; // New
    } as;
};

// --- Constructor Functions ---
Expr* newAssignExpr(Token name, Expr* value);
Expr* newLogicalExpr(Expr* left, Token oper, Expr* right);
Expr* newBinaryExpr(Expr* left, Token oper, Expr* right);
Expr* newCallExpr(Expr* callee, Token paren, int arg_count, Expr** arguments);
Expr* newGroupingExpr(Expr* expression);
Expr* newLiteralNumberExpr(double value);
Expr* newLiteralBooleanExpr(bool value);
Expr* newLiteralStringExpr(char* value); // Assumes value is already managed/copied if needed
Expr* newLiteralNilExpr();
Expr* newUnaryExpr(Token oper, Expr* right);
Expr* newVariableExpr(Token name);

// --- Memory Management ---
void freeExpr(Expr* expr);

// --- Debugging ---
// (Optional but helpful) Function to print AST representation
char* printExpr(Expr* expr);

#endif
