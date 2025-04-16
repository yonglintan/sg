#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "scanner.h"
#include "expr.h"
#include "parser.h"
#include "interpreter.h"

static bool hadError = false;
static bool hasRuntimeError = false;

static void report(int line, const char* where, const char* message) {
    fprintf(stderr, "[line %d] Error%s: %s\n", line, where, message);
    hadError = true;
}

static void error(int line, const char* message) {
    report(line, "", message);
}

static void run(const char* source);
static void runFile(const char* path);
static void runPrompt(void);

int main(int argc, char* argv[]) {
    // we support 2 modes now, file and repl
    if (argc > 2) {
        printf("Usage: sg [script]\n");
        exit(64);
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        runPrompt();
    }
    
    return 0;
}

static void runFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }
    
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);
    
    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }
    
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }
    
    buffer[bytesRead] = '\0';
    
    run(buffer);
    
    free(buffer);
    fclose(file);
    
    if (hadError) exit(65);
    if (hasRuntimeError) exit(70);
}

static void runPrompt(void) {
    char line[1024];
    
    for (;;) {
        printf("> ");
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        // also allow for exit() 
        if (strcmp(line, "exit()\n") == 0) {
            printf("Exiting...\n");
            break;
        }
        
        // remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        run(line);
        
        // reset flags
        hadError = false;
        hasRuntimeError = false;
        resetRuntimeError();
    }
}

static void run(const char* source) {
    Scanner scanner;
    initScanner(&scanner, source);
    
    int tokenCount = 0;
    Token* tokens = scanTokens(&scanner, &tokenCount);

    Parser parser;
    initParser(&parser, tokens, tokenCount);
    Expr* expression = parse(&parser);
    
    // if there was a scanning or parsing error, stop here
    if (hadError || hadParserError(&parser) || expression == NULL) {
        freeTokens(tokens, tokenCount);
        return;
    }
    
    // interpret the expression
    Expr* result = interpret(expression);
    
    hasRuntimeError = hadRuntimeError();
    
    // if no error we can print a result
    if (!hasRuntimeError && result != NULL) {
        char* value = printExpr(result);
        printf("%s\n", value);
        free(value);
        freeExpr(result);
    }
    
    freeExpr(expression);
    freeTokens(tokens, tokenCount);
}
