#include "parser.h"
#include "../ast/expr.h"
#include "../ast/stmt.h"
#include "scanner.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Expr* expression(Parser* parser);
static Expr* assignment(Parser* parser);
static Expr* or (Parser * parser);
static Expr* and (Parser * parser);
static Expr* equality(Parser* parser);
static Expr* comparison(Parser* parser);
static Expr* term(Parser* parser);
static Expr* factor(Parser* parser);
static Expr* unary(Parser* parser);
static Expr* call(Parser* parser);
static Expr* primary(Parser* parser);
static Expr* finishCall(Parser* parser, Expr* callee);

static bool match(Parser* parser, TokenType type);
static bool check(Parser* parser, TokenType type);
static Token advance(Parser* parser);
static Token peek(Parser* parser);
static Token previous(Parser* parser);
static bool isAtEnd(Parser* parser);
static Token consume(Parser* parser, TokenType type, const char* message);
static void synchronize(Parser* parser);
static void error(Parser* parser, Token token, const char* message);

static Stmt* declaration(Parser* parser);
static Stmt* function(Parser* parser, const char* kind);
static Stmt* statement(Parser* parser);
static Stmt* forStatement(Parser* parser);
static Stmt* ifStatement(Parser* parser);
static Stmt* printStatement(Parser* parser);
static Stmt* whileStatement(Parser* parser);
static Stmt* returnStatement(Parser* parser);
static Stmt* expressionStatement(Parser* parser);
static Stmt* varDeclaration(Parser* parser);
static StmtList* block(Parser* parser); // Returns a list for BlockStmt

void initParser(Parser* parser, Token* tokens, int count) {
    parser->tokens = tokens;
    parser->count = count;
    parser->current = 0;
    // overall result of parsing, whether we found any errors or not
    parser->hadError = false;
    // panic mode is the temp state, reset after synchronization happens, so we can handle more errors
    parser->panicMode = false;
}

StmtList* parse(Parser* parser) {
    StmtList* statements = NULL;
    StmtList* tail = NULL;

    while (!isAtEnd(parser)) {
        Stmt* decl = declaration(parser); // declaration handles synchronization on error
        if (parser->hadError) {
            // If an error occurred anywhere during declaration parsing,
            // free everything parsed so far at this top level and stop.
            // Internal structures (like block lists) should have been freed
            // by the function where the error occurred (e.g., block()).
            freeStmtList(statements);
            return NULL;
        }
        if (decl != NULL) {
            // Add the successfully parsed statement to the list
            StmtList* newNode = newStmtList(decl, NULL);
            if (statements == NULL) {
                statements = newNode;
                tail = newNode;
            } else {
                tail->next = newNode;
                tail = newNode;
            }
        }
        // If decl is NULL but no error, it might be an empty input or handled case.
    }

    return statements;
}

bool hadParserError(Parser* parser) {
    return parser->hadError;
}

static void errorAt(Parser* parser, Token token, const char* message) {
    if (parser->panicMode) return; // if we're already in panic mode, don't do anything
    parser->panicMode = true;
    parser->hadError = true;

    fprintf(stderr, "[line %d] Aiyo problem sia:", token.line);

    if (token.type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token.type == TOKEN_ERROR) {
        // Nothing - error token already has message
        // fprintf(stderr, ""); // Ensure ':' is added
    } else {
        fprintf(stderr, " at '%.*s'", token.length, token.start);
    }

    fprintf(stderr, ": %s\n", message);
}

static void error(Parser* parser, Token token, const char* message) {
    errorAt(parser, token, message);
}

static void errorAtCurrent(Parser* parser, const char* message) {
    errorAt(parser, peek(parser), message);
}

static bool isAtEnd(Parser* parser) {
    return peek(parser).type == TOKEN_EOF;
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
    if (type == TOKEN_SEMICOLON && check(parser, TOKEN_LAH)) {
        return advance(parser);
    }

    // message = error message
    if (check(parser, type)) return advance(parser);
    errorAtCurrent(parser, message);

    // can either return an error token or handle differently?
    // For now, return previous potentially? idk
    return peek(parser);
}

// Synchronize the parser after an error
static void synchronize(Parser* parser) {
    parser->panicMode = false;

    while (!isAtEnd(parser)) {
        if (previous(parser).type == TOKEN_SEMICOLON) return;

        switch (peek(parser).type) {
            case TOKEN_CLASS:
            case TOKEN_HOWDO:
            case TOKEN_CHOPE:
            case TOKEN_DO_AGAIN_FROM:
            case TOKEN_CAN:
            case TOKEN_KEEP_DOING:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return; // Start parsing from the next likely statement beginning
            default:
                // Do nothing, just advance
                break;
        }
        advance(parser);
    }
}

// --- Statement Parsing Rules ---

// declaration -> funDecl | varDecl | statement
static Stmt* declaration(Parser* parser) {
    Stmt* stmt = NULL;
    if (match(parser, TOKEN_HOWDO)) {
        stmt = function(parser, "howdo");
    } else if (match(parser, TOKEN_CHOPE)) {
        stmt = varDeclaration(parser);
    } else {
        stmt = statement(parser);
    }

    // If an error occurred during parsing the declaration/statement,
    // enter panic mode and synchronize.
    if (parser->hadError) {
        synchronize(parser);
        // Return NULL to signal the error to the main parse loop
        return NULL;
    }

    return stmt;
}

// function ->  "(" parameters? ")"
static Stmt* function(Parser* parser, const char* kind) {
    char message[64];
    snprintf(message, sizeof(message), "Where the %s name ah?", kind);
    Token name = consume(parser, TOKEN_IDENTIFIER, message);
    if (parser->hadError) return NULL;

    consume(parser, TOKEN_LEFT_PAREN, "Aiyo, after function name must have '(' one lah!");
    if (parser->hadError) return NULL;

    // Parse parameters
    Token* parameters = NULL;
    int param_count = 0;

    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            if (param_count >= 255) {
                error(parser, peek(parser), "Walao, too many parameters sia. Max 255 can already!");
                free(parameters);
                return NULL;
            }

            Token param = consume(parser, TOKEN_IDENTIFIER, "Eh where your parameter name sia?");
            if (parser->hadError) {
                free(parameters);
                return NULL;
            }

            Token* new_params = (Token*)realloc(parameters, sizeof(Token) * (param_count + 1));
            if (new_params == NULL) {
                error(parser, param, "Memory problem lah, cannot allocate for parameters ok.");
                free(parameters);
                return NULL;
            }
            parameters = new_params;

            parameters[param_count] = param;
            param_count++;
        } while (match(parser, TOKEN_COMMA));
    }

    consume(parser, TOKEN_RIGHT_PAREN, "Aiyo, after parameters must close with ')' leh!");
    if (parser->hadError) {
        free(parameters);
        return NULL;
    }

    // Removed: Token leftBrace = consume(parser, TOKEN_LEFT_BRACE, ...);
    StmtList* body = block(parser);
    if (parser->hadError || body == NULL) {
        free(parameters);
        return NULL;
    }

    return newFunctionStmt(name, param_count, parameters, body);
}

// statement -> exprStmt | ifStmt | printStmt | returnStmt | block
static Stmt* statement(Parser* parser) {
    if (match(parser, TOKEN_DO_AGAIN_FROM)) {
        return forStatement(parser);
    }
    if (match(parser, TOKEN_CAN)) {
        return ifStatement(parser);
    }
    if (match(parser, TOKEN_PRINT)) {
        return printStatement(parser);
    }
    if (match(parser, TOKEN_KEEP_DOING)) {
        return whileStatement(parser);
    }
    if (match(parser, TOKEN_RETURN)) {
        return returnStatement(parser);
    }
    if (check(parser, TOKEN_LEFT_BRACE)) {
        StmtList* blockStmts = block(parser); // block() handles {} and returns list
        // If block parsing failed (returned NULL), propagate NULL
        if (blockStmts == NULL && parser->hadError) {
            return NULL;
        }
        // Even if the block is empty (blockStmts is NULL), create the BlockStmt node
        return newBlockStmt(blockStmts);
    }
    // Default to an expression statement
    return expressionStatement(parser);
}

// forStmt -> "for" "(" ( varDecl | exprStmt | ";" ) expression? ";" expression? ")" statement ;
static Stmt* forStatement(Parser* parser) {
    Token leftParen = consume(parser, TOKEN_LEFT_PAREN, "After 'for' must have '(' one leh!");
    if (parser->hadError || leftParen.type == TOKEN_ERROR) return NULL;

    Stmt* initializer;
    if (match(parser, TOKEN_SEMICOLON)) {
        initializer = NULL;
    } else if (match(parser, TOKEN_CHOPE)) {
        initializer = varDeclaration(parser);
        if (parser->hadError) return NULL;
    } else {
        initializer = expressionStatement(parser);
        if (parser->hadError) return NULL;
    }

    Expr* condition = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        condition = expression(parser);
        if (parser->hadError) return NULL;
    }
    consume(parser, TOKEN_SEMICOLON, "Loop condition end liao, must put ';'.");
    if (parser->hadError) return NULL;

    Expr* increment = NULL;
    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        increment = expression(parser);
        if (parser->hadError) return NULL;
    }
    consume(parser, TOKEN_RIGHT_PAREN, "After 'for' must have ')' leh!");
    if (parser->hadError) return NULL;

    Stmt* body = statement(parser);
    if (parser->hadError) return NULL;

    if (increment != NULL) {
        StmtList* incrNode = newStmtList(newExpressionStmt(increment), NULL);
        body = newBlockStmt(newStmtList(body, incrNode));
    }

    if (condition == NULL) {
        condition = newLiteralBooleanExpr(true);
    }
    body = newWhileStmt(condition, body);

    if (initializer != NULL) {
        StmtList* bodyNode = newStmtList(body, NULL);
        body = newBlockStmt(newStmtList(initializer, bodyNode));
    }

    return body;
}

// returnStmt -> "return" expression? ";"
static Stmt* returnStatement(Parser* parser) {
    Token keyword = previous(parser);

    Expr* value = NULL;
    if (!check(parser, TOKEN_SEMICOLON)) {
        value = expression(parser);
        if (parser->hadError) return NULL;
    }

    consume(parser, TOKEN_SEMICOLON, "Aiyo return value means finish already, must end with ';'.");

    return newReturnStmt(keyword, value);
}

// ifStmt -> "if" "(" expression ")" statement ( "else" statement )?
static Stmt* ifStatement(Parser* parser) {
    Token leftParen = consume(parser, TOKEN_LEFT_PAREN, "After 'if' must have '(' leh!");
    if (parser->hadError || leftParen.type == TOKEN_ERROR)
        return NULL; // Error consuming left parenthesis
    Expr* condition = expression(parser);
    if (parser->hadError)
        return NULL; // Propagate error
    Token rightParen = consume(parser, TOKEN_RIGHT_PAREN, "If condition finish liao, where your ')' ah?");
    if (parser->hadError || rightParen.type == TOKEN_ERROR)
        return NULL; // Error consuming right parenthesis

    Stmt* thenBranch = statement(parser);
    if (parser->hadError) {
        return NULL;
    }
    Stmt* elseBranch = NULL;
    if (match(parser, TOKEN_CANNOT)) {
        elseBranch = statement(parser);
        if (parser->hadError)
            return NULL;
    }

    return newIfStmt(condition, thenBranch, elseBranch);
}

// printStmt -> "print" expression
static Stmt* printStatement(Parser* parser) {
    Expr* value = expression(parser); // Parse the expression to print
    if (parser->hadError) return NULL; // Propagate error
    Token semicolon = consume(parser, TOKEN_SEMICOLON, "You print already never put ';'? How can?");
    if (semicolon.type == TOKEN_ERROR) return NULL; // Error consuming semicolon
    return newPrintStmt(value);
}

// whileStmt -> "while" "(" expression ")" statement ;
static Stmt* whileStatement(Parser* parser) {
    Token leftParen = consume(parser, TOKEN_LEFT_PAREN, "After 'while' must have '(' leh!");
    if (parser->hadError || leftParen.type == TOKEN_ERROR) return NULL; // Error consuming parenthesis
    Expr* condition = expression(parser);
    if (parser->hadError) return NULL;
    Token rightParen = consume(parser, TOKEN_RIGHT_PAREN, "Condition close with ')' leh, don't forget.");
    if (parser->hadError || rightParen.type == TOKEN_ERROR) return NULL; // Error consuming parenthesis
    Stmt* body = statement(parser);
    if (parser->hadError) return NULL;

    return newWhileStmt(condition, body);
}

// exprStmt -> expression
static Stmt* expressionStatement(Parser* parser) {
    Expr* expr = expression(parser); // Parse the expression
    if (parser->hadError) return NULL; // Propagate error
    Token semicolon = consume(parser, TOKEN_SEMICOLON, "Expression done liao, remember your ';'!");
    if (semicolon.type == TOKEN_ERROR) return NULL; // Error consuming semicolon
    return newExpressionStmt(expr);
}

// varDecl -> "var" IDENTIFIER ( "=" expression )
static Stmt* varDeclaration(Parser* parser) {
    Token name = consume(parser, TOKEN_IDENTIFIER, "Eh hello, where the variable name?");
    if (name.type == TOKEN_ERROR) return NULL;

    Expr* initializer = NULL;
    if (match(parser, TOKEN_EQUAL)) {
        initializer = expression(parser);
        if (parser->hadError) {
            // If parsing initializer failed, free it (if partially created)
            // Note: expression() doesn't return partial results on error currently
            // freeExpr(initializer); // Not needed based on current expr parsing
            return NULL; // Propagate error
        }
    }

    Token semicolon = consume(parser, TOKEN_SEMICOLON, "After declare variable must have ';' leh.");
    if (semicolon.type == TOKEN_ERROR) {
        // If semicolon fails, we might have a valid initializer expression parsed
        freeExpr(initializer); // Clean up the parsed initializer
        return NULL;
    }
    return newVarStmt(name, initializer);
}

// block -> "{" declaration* "}" ;
static StmtList* block(Parser* parser) {
    // Consume the opening brace '{'
    if (!match(parser, TOKEN_LEFT_BRACE)) { // Should be checked before calling statement()
        errorAtCurrent(parser, "Wah, you never open with '{' ah? Cannot start block like this!");
        return NULL;
    }

    StmtList* statements = NULL;
    StmtList* tail = NULL;

    // Parse declarations inside the block until '}' or EOF
    while (!check(parser, TOKEN_RIGHT_BRACE) && !isAtEnd(parser)) {
        Stmt* decl = declaration(parser); // declaration handles its own synchronization

        if (parser->hadError) {
            // If declaration failed within the block, free the list built for this block.
            freeStmtList(statements);
            statements = NULL; // Mark as freed
            // Don't return yet, try to find the closing brace
            break; // Exit the loop to find '}'
        }

        if (decl != NULL) {
            StmtList* newNode = newStmtList(decl, NULL);
            if (statements == NULL) {
                statements = newNode;
                tail = newNode;
            } else {
                tail->next = newNode;
                tail = newNode;
            }
        }
    }

    consume(parser, TOKEN_RIGHT_BRACE, "After block must close with '}' ok?");

    // If we exited the loop due to an error OR failed to consume '}', cleanup & return NULL
    if (parser->hadError) { // Check error flag *after* trying to consume brace
        // If closingBrace itself caused an error, statements might not be freed yet.
        // If the error was inside the loop, statements should be NULL now.
        if (statements != NULL) {
            freeStmtList(statements);
        }
        return NULL;
    }

    return statements;
}

// --- Expression Parsing Rules ---

// expression -> assignment ;
static Expr* expression(Parser* parser) {
    return assignment(parser); // Use assignment as the top-level expression rule
}

// assignment -> IDENTIFIER "=" assignment | logic_or ;
// eg: a = b = c;
static Expr* assignment(Parser* parser) {
    // Parse the LHS. It might be an identifier, or something else.
    Expr* expr = or (parser);
    if (parser->hadError) return NULL;

    if (match(parser, TOKEN_EQUAL)) {
        Token equals = previous(parser);
        Expr* value = assignment(parser); // Parse the RHS recursively
        if (parser->hadError) {
            freeExpr(expr); // Free the LHS if RHS parsing failed
            return NULL;
        }

        if (expr->type == EXPR_VARIABLE) {
            Token name = expr->as.variable.name;
            Expr* assignNode = newAssignExpr(name, value);
            freeExpr(expr); // free the variable expression since we're replacing it
            return assignNode;
        } else {
            error(parser, equals, "Invalid assignment target.");
            freeExpr(value);
            freeExpr(expr);
            return NULL;
        }
    }

    // If no '=' was matched, just return the expression parsed by or()
    return expr;
}

// logic_or -> logic_and ( "or" logic_and )* ;
static Expr* or (Parser * parser) {
    Expr* expr = and(parser);
    if (parser->hadError) return NULL;

    while (match(parser, TOKEN_OR)) {
        Token oper = previous(parser);
        Expr* right = and(parser);
        if (parser->hadError) {
            freeExpr(expr);
            return NULL;
        }
        expr = newLogicalExpr(expr, oper, right);
    }

    return expr;
}

// logic_and -> equality ( "and" equality )* ;
static Expr* and (Parser * parser) {
    Expr* expr = equality(parser);
    if (parser->hadError) return NULL;

    while (match(parser, TOKEN_AND)) {
        Token oper = previous(parser);
        Expr* right = equality(parser);
        if (parser->hadError) {
            freeExpr(expr);
            return NULL;
        }
        expr = newLogicalExpr(expr, oper, right);
    }

    return expr;
}

// equality -> comparison ( ( "!=" | "==" ) comparison )* ;
static Expr* equality(Parser* parser) {
    Expr* expr = comparison(parser);
    if (parser->hadError) return NULL;

    while (match(parser, TOKEN_BANG_EQUAL) || match(parser, TOKEN_EQUAL_EQUAL)) {
        Token operator= previous(parser);
        Expr* right = comparison(parser);
        if (parser->hadError) {
            freeExpr(expr);
            return NULL;
        }
        expr = newBinaryExpr(expr, operator, right);
    }
    return expr;
}

// comparison -> term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
static Expr* comparison(Parser* parser) {
    Expr* expr = term(parser);
    if (parser->hadError) return NULL;

    while (match(parser, TOKEN_GREATER) || match(parser, TOKEN_GREATER_EQUAL) || match(parser, TOKEN_LESS) || match(parser, TOKEN_LESS_EQUAL)) {
        Token operator= previous(parser);
        Expr* right = term(parser);
        if (parser->hadError) {
            freeExpr(expr);
            return NULL;
        }
        expr = newBinaryExpr(expr, operator, right);
    }
    return expr;
}

// term -> factor ( ( "-" | "+" ) factor )* ;
static Expr* term(Parser* parser) {
    Expr* expr = factor(parser);
    if (parser->hadError) return NULL;

    while (match(parser, TOKEN_MINUS) || match(parser, TOKEN_PLUS)) {
        Token operator= previous(parser);
        Expr* right = factor(parser);
        if (parser->hadError) {
            freeExpr(expr);
            return NULL;
        }
        expr = newBinaryExpr(expr, operator, right);
    }
    return expr;
}

// factor -> unary ( ( "/" | "*" ) unary )* ;
static Expr* factor(Parser* parser) {
    Expr* expr = unary(parser);
    if (parser->hadError) return NULL;

    while (match(parser, TOKEN_SLASH) || match(parser, TOKEN_STAR)) {
        Token operator= previous(parser);
        Expr* right = unary(parser);
        if (parser->hadError) {
            freeExpr(expr);
            return NULL;
        }
        expr = newBinaryExpr(expr, operator, right);
    }
    return expr;
}

// unary -> ( "!" | "-" ) unary | call ;
static Expr* unary(Parser* parser) {
    if (match(parser, TOKEN_BANG) || match(parser, TOKEN_MINUS)) {
        Token operator= previous(parser);
        Expr* right = unary(parser);
        if (parser->hadError) return NULL;
        return newUnaryExpr(operator, right);
    }
    return call(parser);
}

// call -> primary ( "(" arguments? ")" )* ;
static Expr* call(Parser* parser) {
    Expr* expr = primary(parser);
    if (parser->hadError) return NULL;

    while (match(parser, TOKEN_LEFT_PAREN)) {
        expr = finishCall(parser, expr);
        if (parser->hadError) {
            freeExpr(expr);
            return NULL;
        }
    }

    return expr;
}

// Parse arguments for a function call
static Expr* finishCall(Parser* parser, Expr* callee) {
    Expr** arguments = NULL;
    int arg_count = 0;

    if (!check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            if (arg_count >= 255) {
                error(parser, peek(parser), "Can't have more than 255 arguments.");
                for (int i = 0; i < arg_count; i++) {
                    freeExpr(arguments[i]);
                }
                free(arguments);
                return NULL;
            }

            Expr* argument = expression(parser);
            if (parser->hadError) {
                for (int i = 0; i < arg_count; i++) {
                    freeExpr(arguments[i]);
                }
                free(arguments);
                return NULL;
            }

            Expr** new_args = (Expr**)realloc(arguments, sizeof(Expr*) * (arg_count + 1));
            if (new_args == NULL) {
                error(parser, peek(parser), "Memory error allocating arguments.");
                for (int i = 0; i < arg_count; i++) {
                    freeExpr(arguments[i]);
                }
                free(arguments);
                freeExpr(argument);
                return NULL;
            }
            arguments = new_args;

            arguments[arg_count] = argument;
            arg_count++;
        } while (match(parser, TOKEN_COMMA));
    }

    Token paren = consume(parser, TOKEN_RIGHT_PAREN, "After argument list must have ')'.");
    if (parser->hadError) {
        for (int i = 0; i < arg_count; i++) {
            freeExpr(arguments[i]);
        }
        free(arguments);
        return NULL;
    }

    return newCallExpr(callee, paren, arg_count, arguments);
}

// primary -> NUMBER | STRING | "true" | "false" | "nil" | IDENTIFIER | "(" expression ")" ;
static Expr* primary(Parser* parser) {
    if (match(parser, TOKEN_WRONG)) return newLiteralBooleanExpr(false);
    if (match(parser, TOKEN_CORRECT)) return newLiteralBooleanExpr(true);
    if (match(parser, TOKEN_NIL)) return newLiteralNilExpr();

    if (match(parser, TOKEN_NUMBER)) {
        double value = strtod(previous(parser).start, NULL);
        return newLiteralNumberExpr(value);
    }

    if (match(parser, TOKEN_STRING)) {
        const char* strStart = previous(parser).start + 1;
        int length = previous(parser).length - 2;
        char* value = (char*)malloc(length + 1);
        if (value == NULL) {
            error(parser, previous(parser), "Memory error copying string literal.");
            return NULL;
        }
        strncpy(value, strStart, length);
        value[length] = '\0';
        Expr* literal = newLiteralStringExpr(value);
        free(value);
        return literal;
    }

    if (match(parser, TOKEN_IDENTIFIER)) {
        // Create a variable expression node using the identifier token
        return newVariableExpr(previous(parser));
    }

    if (match(parser, TOKEN_LEFT_PAREN)) {
        Expr* expr = expression(parser);
        if (parser->hadError) return NULL;
        Token closingParen = consume(parser, TOKEN_RIGHT_PAREN, "After expression must close with ')'.");
        if (closingParen.type == TOKEN_ERROR) {
            freeExpr(expr);
            return NULL;
        }
        return newGroupingExpr(expr);
    }

    // If none of the above match, it's an error
    errorAtCurrent(parser, "Alamak! Expression where?");
    return NULL;
}