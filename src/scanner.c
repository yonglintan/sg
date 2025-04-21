#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scanner.h"

static Token makeToken(Scanner* scanner, TokenType type) {
    Token token;
    token.type = type;
    token.start = scanner->start;
    token.length = (int)(scanner->current - scanner->start);
    token.line = scanner->line;
    return token;
}

static Token errorToken(Scanner* scanner, const char* message) {
    Token token;
    token.type = TOKEN_ERROR;

    // Allocate a copy of the message so it persists
    char* messageCopy = strdup(message);
    if (messageCopy == NULL) {
        // Fall back to a static message if allocation fails
        token.start = "Memory error";
    } else {
        token.start = messageCopy;
    }

    token.length = (int)strlen(token.start);
    token.line = scanner->line;
    return token;
}

static bool isAtEnd(Scanner* scanner) {
    return *scanner->current == '\0';
}

static char advance(Scanner* scanner) {
    return *scanner->current++;
}

static bool match(Scanner* scanner, char expected) {
    if (isAtEnd(scanner)) return false;
    if (*scanner->current != expected) return false;

    scanner->current++;
    return true;
}

static char peek(Scanner* scanner) {
    return *scanner->current;
}

static char peekNext(Scanner* scanner) {
    if (isAtEnd(scanner)) return '\0';
    return scanner->current[1];
}

static void skipWhitespace(Scanner* scanner) {
    for (;;) {
        char c = peek(scanner);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance(scanner);
                break;
            case '\n':
                scanner->line++;
                advance(scanner);
                break;
            case '/':
                if (peekNext(scanner) == '/') {
                    // A comment goes until the end of the line.
                    while (peek(scanner) != '\n' && !isAtEnd(scanner)) {
                        advance(scanner);
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
}

static Token number(Scanner* scanner) {
    while (isDigit(peek(scanner)))
        advance(scanner);

    // Look for a fractional part.
    if (peek(scanner) == '.' && isDigit(peekNext(scanner))) {
        // Consume the ".".
        advance(scanner);

        while (isDigit(peek(scanner)))
            advance(scanner);
    }

    return makeToken(scanner, TOKEN_NUMBER);
}

static Token string(Scanner* scanner) {
    while (peek(scanner) != '"' && !isAtEnd(scanner)) {
        if (peek(scanner) == '\n') scanner->line++;
        advance(scanner);
    }

    if (isAtEnd(scanner)) return errorToken(scanner, "Unterminated string.");

    // The closing quote.
    advance(scanner);
    return makeToken(scanner, TOKEN_STRING);
}

static TokenType checkKeyword(Scanner* scanner, int start, int length,
                              const char* rest, TokenType type) {
    if (scanner->current - scanner->start == start + length && memcmp(scanner->start + start, rest, length) == 0) {
        return type;
    }

    return TOKEN_IDENTIFIER;
}

static TokenType identifierType(Scanner* scanner) {
    switch (scanner->start[0]) {
        case 'a':
            return checkKeyword(scanner, 1, 2, "nd", TOKEN_AND);
        // case 'c': return checkKeyword(scanner, 1, 4, "lass", TOKEN_CLASS);
        case 'c':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 'a':
                        if (scanner->current - scanner->start > 2 && scanner->start[2] == 'n') {
                            if (scanner->current - scanner->start > 5 && scanner->start[3] == 'n' && scanner->start[4] == 'o' && scanner->start[5] == 't') {
                                return TOKEN_CANNOT;
                            }
                            return TOKEN_CAN;
                        }
                        break;
                    case 'o':
                        return checkKeyword(scanner, 2, 5, "rrect", TOKEN_CORRECT);
                        // case 'l':
                        //     return checkKeyword(scanner, 2, 3, "ass", TOKEN_CLASS);
                }
            }
            break;
        case 'd':
            return checkKeyword(scanner, 2, 1, "o again", TOKEN_DO_AGAIN_FROM);
        case 'f':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    // case 'w':
                    //     return checkKeyword(scanner, 1, 4, "rong", TOKEN_WRONG);
                }
            }
            break;
        case 'h':
            return checkKeyword(scanner, 1, 4, "owdo", TOKEN_HOWDO);
        case 'k':
            return checkKeyword(scanner, 2, 3, "eep doing", TOKEN_KEEP_DOING);
        case 'l':
            return checkKeyword(scanner, 1, 2, "ah", TOKEN_LAH);
        case 'n':
            return checkKeyword(scanner, 1, 2, "il", TOKEN_NIL);
        case 'o':
            return checkKeyword(scanner, 1, 1, "r", TOKEN_OR);
        case 'p':
            return checkKeyword(scanner, 1, 4, "rint", TOKEN_PRINT);
        case 'r':
            return checkKeyword(scanner, 1, 5, "eturn", TOKEN_RETURN);

        case 's':
            return checkKeyword(scanner, 1, 4, "uper", TOKEN_SUPER);
        case 't':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 'h':
                        return checkKeyword(scanner, 2, 2, "is", TOKEN_THIS);
                        // case 'c':
                        //     return checkKeyword(scanner, 1, 6, "orrect", TOKEN_CORRECT);
                }
            }
            break;
        case 'g':
            return checkKeyword(scanner, 1, 2, "ot", TOKEN_GOT);
        case 'w':
            if (scanner->current - scanner->start > 1) {
                switch (scanner->start[1]) {
                    case 'r':
                        return checkKeyword(scanner, 2, 3, "ong", TOKEN_WRONG);
                }
            }
            break;
    }

    return TOKEN_IDENTIFIER;
}

static Token identifier(Scanner* scanner) {
    while (isAlphaNumeric(peek(scanner)))
        advance(scanner);

    return makeToken(scanner, identifierType(scanner));
}

static Token scanToken(Scanner* scanner) {
    skipWhitespace(scanner);

    scanner->start = scanner->current;

    if (isAtEnd(scanner)) return makeToken(scanner, TOKEN_EOF);

    // check for multi-word keywords
    const char* curr = scanner->current;
    int remaining = 0;

    remaining = 0;
    while (curr[remaining] != '\0')
        remaining++;

    if (remaining >= 10 && strncmp(curr, "keep", 4) == 0 && curr[4] == ' ' && strncmp(curr + 5, "doing", 5) == 0 && (curr[10] == '\0' || !isAlphaNumeric(curr[10]))) {
        scanner->current += 10;
        return makeToken(scanner, TOKEN_KEEP_DOING);
    }

    remaining = 0;
    while (curr[remaining] != '\0')
        remaining++;

    if (remaining >= 13 && strncmp(curr, "do", 2) == 0 && curr[2] == ' ' && strncmp(curr + 3, "again", 5) == 0 && curr[8] == ' ' && strncmp(curr + 9, "from", 4) == 0 && (curr[13] == '\0' || !isAlphaNumeric(curr[13]))) {
        scanner->current += 13;
        return makeToken(scanner, TOKEN_DO_AGAIN_FROM);
    }

    char c = advance(scanner);

    if (isAlpha(c)) return identifier(scanner);
    if (isDigit(c)) return number(scanner);

    switch (c) {
        case '(':
            return makeToken(scanner, TOKEN_LEFT_PAREN);
        case ')':
            return makeToken(scanner, TOKEN_RIGHT_PAREN);
        case '{':
            return makeToken(scanner, TOKEN_LEFT_BRACE);
        case '}':
            return makeToken(scanner, TOKEN_RIGHT_BRACE);
        case ';':
            return makeToken(scanner, TOKEN_SEMICOLON);
        case ',':
            return makeToken(scanner, TOKEN_COMMA);
        case '.':
            return makeToken(scanner, TOKEN_DOT);
        case '-':
            return makeToken(scanner, TOKEN_MINUS);
        case '+':
            return makeToken(scanner, TOKEN_PLUS);
        case '/':
            return makeToken(scanner, TOKEN_SLASH);
        case '*':
            return makeToken(scanner, TOKEN_STAR);
        case '!':
            return makeToken(scanner, match(scanner, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '=':
            return makeToken(scanner, match(scanner, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '<':
            return makeToken(scanner, match(scanner, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
        case '>':
            return makeToken(scanner, match(scanner, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
        case '"':
            return string(scanner);
    }

    char errorMsg[64];
    snprintf(errorMsg, sizeof(errorMsg), "Unexpected character: '%c'", c);
    return errorToken(scanner, errorMsg);
}

void initScanner(Scanner* scanner, const char* source) {
    scanner->start = source;
    scanner->current = source;
    scanner->line = 1;
}

Token* scanTokens(Scanner* scanner, int* tokenCount) {
    // Preallocate some tokens to start with
    int capacity = 8;
    Token* tokens = (Token*)malloc(sizeof(Token) * capacity);
    int count = 0;

    for (;;) {
        // Resize array if needed
        if (count >= capacity) {
            capacity = capacity * 2;
            tokens = (Token*)realloc(tokens, sizeof(Token) * capacity);
        }

        Token token = scanToken(scanner);
        tokens[count++] = token;

        if (token.type == TOKEN_EOF) break;
    }

    *tokenCount = count;
    return tokens;
}

void printToken(Token token) {
    printf("%4d ", token.line);

    switch (token.type) {
        case TOKEN_LEFT_PAREN:
            printf("LEFT_PAREN");
            break;
        case TOKEN_RIGHT_PAREN:
            printf("RIGHT_PAREN");
            break;
        case TOKEN_LEFT_BRACE:
            printf("LEFT_BRACE");
            break;
        case TOKEN_RIGHT_BRACE:
            printf("RIGHT_BRACE");
            break;
        case TOKEN_COMMA:
            printf("COMMA");
            break;
        case TOKEN_DOT:
            printf("DOT");
            break;
        case TOKEN_MINUS:
            printf("MINUS");
            break;
        case TOKEN_PLUS:
            printf("PLUS");
            break;
        case TOKEN_SEMICOLON:
            printf("SEMICOLON");
            break;
        case TOKEN_SLASH:
            printf("SLASH");
            break;
        case TOKEN_STAR:
            printf("STAR");
            break;
        case TOKEN_BANG:
            printf("BANG");
            break;
        case TOKEN_BANG_EQUAL:
            printf("BANG_EQUAL");
            break;
        case TOKEN_EQUAL:
            printf("EQUAL");
            break;
        case TOKEN_EQUAL_EQUAL:
            printf("EQUAL_EQUAL");
            break;
        case TOKEN_GREATER:
            printf("GREATER");
            break;
        case TOKEN_GREATER_EQUAL:
            printf("GREATER_EQUAL");
            break;
        case TOKEN_LESS:
            printf("LESS");
            break;
        case TOKEN_LESS_EQUAL:
            printf("LESS_EQUAL");
            break;
        case TOKEN_IDENTIFIER:
            printf("IDENTIFIER");
            break;
        case TOKEN_STRING:
            printf("STRING");
            break;
        case TOKEN_NUMBER:
            printf("NUMBER");
            break;
        case TOKEN_AND:
            printf("AND");
            break;
        case TOKEN_CLASS:
            printf("CLASS");
            break;
        case TOKEN_CANNOT:
            printf("CANNOT");
            break;
        case TOKEN_WRONG:
            printf("WRONG");
            break;
        case TOKEN_DO_AGAIN_FROM:
            printf("DO AGAIN FROM");
            break;
        case TOKEN_HOWDO:
            printf("HOWDO");
            break;
        case TOKEN_CAN:
            printf("IF");
            break;
        case TOKEN_NIL:
            printf("NIL");
            break;
        case TOKEN_OR:
            printf("OR");
            break;
        case TOKEN_PRINT:
            printf("PRINT");
            break;
        case TOKEN_RETURN:
            printf("RETURN");
            break;
        case TOKEN_SUPER:
            printf("SUPER");
            break;
        case TOKEN_THIS:
            printf("THIS");
            break;
        case TOKEN_CORRECT:
            printf("CORRECT");
            break;
        case TOKEN_GOT:
            printf("GOT");
            break;
        case TOKEN_KEEP_DOING:
            printf("KEEP DOING");
            break;
        case TOKEN_ERROR:
            printf("ERROR");
            break;
        case TOKEN_EOF:
            printf("EOF");
            break;
        default:
            printf("UNKNOWN");
            break;
    }

    // Print the lexeme for identifiers, strings, and numbers
    if (token.type == TOKEN_IDENTIFIER || token.type == TOKEN_STRING || token.type == TOKEN_NUMBER) {
        printf(" '%.*s'", token.length, token.start);
    } else if (token.type == TOKEN_ERROR) {
        printf(" %s", token.start);
    }

    printf("\n");
}

void freeTokens(Token* tokens, int tokenCount) {
    for (int i = 0; i < tokenCount; i++) {
        if (tokens[i].type == TOKEN_ERROR) {
            // Free the allocated error message
            free((void*)tokens[i].start);
        }
    }
    free(tokens);
}
