#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "parser.h"
#include "scanner.h"
#include "expr.h"

static Expr* expression(Parser* parser);
static Expr* equality(Parser* parser);
static Expr* comparison(Parser* parser);
static Expr* term(Parser* parser);
static Expr* factor(Parser* parser);
static Expr* unary(Parser* parser);
static Expr* primary(Parser* parser);

static bool match(Parser* parser, TokenType type);
static bool check(Parser* parser, TokenType type);
static Token advance(Parser* parser);
static Token peek(Parser* parser);
static Token previous(Parser* parser);
static bool isAtEnd(Parser* parser);
static Token consume(Parser* parser, TokenType type, const char* message);
static void synchronize(Parser* parser);
static void error(Parser* parser, Token token, const char* message);

void initParser(Parser* parser, Token* tokens, int count) {
    parser->tokens = tokens;
    parser->count = count;
    parser->current = 0;
    parser->hadError = false;
}

Expr* parse(Parser* parser) {
    return expression(parser);
}

bool hadParserError(Parser* parser) {
    return parser->hadError;
}

// Error handling
static bool panicMode = false;

static void errorAt(Parser* parser, Token token, const char* message) {
    if (panicMode) return;
    panicMode = true;
    parser->hadError = true;

    fprintf(stderr, "[line %d] Error", token.line);

    if (token.type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token.type == TOKEN_ERROR) {
        // Nothing
    } else {
        fprintf(stderr, " at '%.*s'", token.length, token.start);
    }

    fprintf(stderr, ": %s\n", message);
}

static void error(Parser* parser, Token token, const char* message) {
    errorAt(parser, token, message);
}

static void errorAtCurrent(Parser* parser, const char* message) {
    errorAt(parser, parser->tokens[parser->current], message);
}

// Helper functions for token handling
static bool isAtEnd(Parser* parser) {
    return parser->tokens[parser->current].type == TOKEN_EOF;
}

static Token previous(Parser* parser) {
    return parser->tokens[parser->current - 1];
}

static Token peek(Parser* parser) {
    return parser->tokens[parser->current];
}

static Token advance(Parser* parser) {
    if (!isAtEnd(parser)) parser->current++;
    return previous(parser);
}

static bool check(Parser* parser, TokenType type) {
    if (isAtEnd(parser)) return false;
    return peek(parser).type == type;
}

static bool match(Parser* parser, TokenType type) {
    if (check(parser, type)) {
        advance(parser);
        return true;
    }
    return false;
}

static Token consume(Parser* parser, TokenType type, const char* message) {
    if (check(parser, type)) return advance(parser);
    
    errorAtCurrent(parser, message);
    return (Token){TOKEN_ERROR, "", 0, 0};
}

// Synchronize the parser after an error to continue parsing
static void synchronize(Parser* parser) {
    panicMode = false;

    while (!isAtEnd(parser)) {
        if (previous(parser).type == TOKEN_SEMICOLON) return;

        switch (peek(parser).type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default:
                // Do nothing
                break;
        }

        advance(parser);
    }
}

// Grammar rule implementations

// expression → equality
static Expr* expression(Parser* parser) {
    return equality(parser);
}

// equality → comparison ( ( "!=" | "==" ) comparison )*
static Expr* equality(Parser* parser) {
    Expr* expr = comparison(parser);

    while (match(parser, TOKEN_BANG_EQUAL) || match(parser, TOKEN_EQUAL_EQUAL)) {
        Token operator = previous(parser);
        Expr* right = comparison(parser);
        expr = (Expr*)newBinaryExpr(expr, operator, right);
    }

    return expr;
}

// comparison → term ( ( ">" | ">=" | "<" | "<=" ) term )*
static Expr* comparison(Parser* parser) {
    Expr* expr = term(parser);

    while (match(parser, TOKEN_GREATER) || match(parser, TOKEN_GREATER_EQUAL) ||
           match(parser, TOKEN_LESS) || match(parser, TOKEN_LESS_EQUAL)) {
        Token operator = previous(parser);
        Expr* right = term(parser);
        expr = (Expr*)newBinaryExpr(expr, operator, right);
    }

    return expr;
}

// term → factor ( ( "-" | "+" ) factor )*
static Expr* term(Parser* parser) {
    Expr* expr = factor(parser);

    while (match(parser, TOKEN_MINUS) || match(parser, TOKEN_PLUS)) {
        Token operator = previous(parser);
        Expr* right = factor(parser);
        expr = (Expr*)newBinaryExpr(expr, operator, right);
    }

    return expr;
}

// factor → unary ( ( "/" | "*" ) unary )*
static Expr* factor(Parser* parser) {
    Expr* expr = unary(parser);

    while (match(parser, TOKEN_SLASH) || match(parser, TOKEN_STAR)) {
        Token operator = previous(parser);
        Expr* right = unary(parser);
        expr = (Expr*)newBinaryExpr(expr, operator, right);
    }

    return expr;
}

// unary → ( "!" | "-" ) unary | primary
static Expr* unary(Parser* parser) {
    if (match(parser, TOKEN_BANG) || match(parser, TOKEN_MINUS)) {
        Token operator = previous(parser);
        Expr* right = unary(parser);
        return (Expr*)newUnaryExpr(operator, right);
    }

    return primary(parser);
}

// primary → NUMBER | STRING | "true" | "false" | "nil" | "(" expression ")"
static Expr* primary(Parser* parser) {
    if (match(parser, TOKEN_FALSE)) return (Expr*)newLiteralBooleanExpr(false);
    if (match(parser, TOKEN_TRUE)) return (Expr*)newLiteralBooleanExpr(true);
    if (match(parser, TOKEN_NIL)) return (Expr*)newLiteralNilExpr();

    if (match(parser, TOKEN_NUMBER)) {
        double value = strtod(parser->tokens[parser->current - 1].start, NULL);
        return (Expr*)newLiteralNumberExpr(value);
    }

    if (match(parser, TOKEN_STRING)) {
        // Extract the string value without quotes
        const char* str = parser->tokens[parser->current - 1].start + 1;
        int length = parser->tokens[parser->current - 1].length - 2;
        char* value = (char*)malloc(length + 1);
        strncpy(value, str, length);
        value[length] = '\0';
        
        Expr* result = (Expr*)newLiteralStringExpr(value);
        free(value);
        return result;
    }

    if (match(parser, TOKEN_LEFT_PAREN)) {
        Expr* expr = expression(parser);
        consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
        return (Expr*)newGroupingExpr(expr);
    }

    // If we get here, we couldn't match a valid expression
    error(parser, parser->tokens[parser->current], "Expect expression.");
    return NULL;
} 