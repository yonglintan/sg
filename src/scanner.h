#ifndef scanner_h
#define scanner_h

#include <stdbool.h>

// Token types
typedef enum {
    // Single-character tokens
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_MINUS,
    TOKEN_PLUS,
    TOKEN_SEMICOLON,
    TOKEN_SLASH,
    TOKEN_STAR,

    // One or two character tokens
    TOKEN_BANG,
    TOKEN_BANG_EQUAL,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,

    // Literals
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,

    // Singlish Keywords
    TOKEN_LAH, // New: statement terminator
    TOKEN_IF_NOT, // New: replaces TOKEN_ELSE
    TOKEN_KEEP_DOING, // New: replaces TOKEN_WHILE
    TOKEN_DO_AGAIN_FROM, // New: replaces TOKEN_FOR
    TOKEN_HOWDO, // New: replaces TOKEN_FUN
    TOKEN_GOT, // New: replaces TOKEN_VAR

    // Keywords
    TOKEN_AND,
    TOKEN_CLASS,
    TOKEN_ELSE,
    TOKEN_FALSE,
    TOKEN_DO_AGAIN_FROM,
    TOKEN_HOWDO,
    TOKEN_IF,
    TOKEN_NIL,
    TOKEN_OR,
    TOKEN_PRINT,
    TOKEN_RETURN,
    TOKEN_SUPER,
    TOKEN_THIS,
    TOKEN_TRUE,
    TOKEN_GOT,
    TOKEN_KEEP_DOING,

    TOKEN_ERROR,
    TOKEN_EOF
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
