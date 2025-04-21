#ifndef parser_h
#define parser_h

#include "../ast/expr.h"
#include "../ast/stmt.h"
#include "scanner.h"

typedef struct {
    Token* tokens;
    int current;
    int count;
    bool hadError;
    bool panicMode;
} Parser;

void initParser(Parser* parser, Token* tokens, int count);

StmtList* parse(Parser* parser);

bool hadParserError(Parser* parser);

#endif