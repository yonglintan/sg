#ifndef scanner_h
#define scanner_h

#include <stdbool.h>

// Token types
typedef enum {
    // Single-character tokens
    TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
    TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
    TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,

    // One or two character tokens
    TOKEN_BANG, TOKEN_BANG_EQUAL,
    TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER, TOKEN_GREATER_EQUAL,
    TOKEN_LESS, TOKEN_LESS_EQUAL,

    // Literals
    TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

    // Keywords
    TOKEN_AND, TOKEN_CLASS, TOKEN_CANNOT, TOKEN_WRONG,
    TOKEN_FOR, TOKEN_HOWDO, TOKEN_CAN, TOKEN_NIL, TOKEN_OR,
    TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
    TOKEN_CORRECT, TOKEN_GOT, TOKEN_WHILE,

    TOKEN_ERROR, TOKEN_EOF,

    TOKEN_LAH
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
} Token;

// Scanner structure
typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

// Initialize scanner with source code
void initScanner(Scanner* scanner, const char* source);

// Scan all tokens from the source
Token* scanTokens(Scanner* scanner, int* tokenCount);

// Free tokens array and any dynamically allocated error messages
void freeTokens(Token* tokens, int tokenCount);

// Get a human-readable representation of token type
const char* tokenTypeToString(TokenType type);

// Print a token for debugging
void printToken(Token token);

#endif 

