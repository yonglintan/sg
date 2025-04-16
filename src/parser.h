#ifndef parser_h
#define parser_h

#include "scanner.h"
#include "expr.h"

typedef struct {
    Token* tokens;
    int current;
    int count;
    bool hadError;
} Parser;

void initParser(Parser* parser, Token* tokens, int count);

Expr* parse(Parser* parser);

bool hadParserError(Parser* parser);

#endif 