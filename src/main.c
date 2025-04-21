#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backend/environment.h"
#include "backend/interpreter.h"
#include "frontend/parser.h"
#include "frontend/resolver.h"
#include "frontend/scanner.h"
#include "runtime/object.h"

static bool hadScanParseError = false;

// static void report(int line, const char* where, const char* message) {
//     fprintf(stderr, "[line %d] Aiyo problem sia%s: %s\n", line, where ? where : "",
//             message);
//     hadScanParseError = true;
// }

static void run(const char* source);
static void runFile(const char* path);
static void runPrompt(void);

int main(int argc, char* argv[]) {
    initInterpreter(); // Initialize global environment, etc.

    if (argc > 2) {
        printf("Usage: sg [script]\n");
        exit(64); // EX_USAGE
    } else if (argc == 2) {
        runFile(argv[1]);
    } else {
        runPrompt();
    }

    freeInterpreter(); // Clean up global environment
    return 0;
}

static void runFile(const char* path) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Alamak, cannot open file \"%s\" sia.\n", path);
        exit(74); // EX_IOERR
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Wah, not enough memory to read \"%s\" leh.\n", path);
        fclose(file);
        exit(74); // EX_IOERR or EX_OSERR
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize && ferror(file)) {
        fprintf(stderr, "Aiyo, cannot read file \"%s\" lah.\n", path);
        free(buffer);
        fclose(file);
        exit(74); // EX_IOERR
    }

    buffer[bytesRead] = '\0';
    fclose(file);

    run(buffer);
    free(buffer);

    if (hadScanParseError) exit(65); // EX_DATAERR
    if (hadRuntimeError()) exit(70); // EX_SOFTWARE
}

static void runPrompt(void) {
    char line[1024];
    printf("REPL mode: (Ctrl+D or exit() to quit)\n");
    for (;;) {
        printf("> "); // Prompt
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\nExiting.\n"); // Ctrl+D
            break;
        }

        // Simple exit command for convenience
        if (strcmp(line, "exit()\n") == 0) {
            printf("Exiting.\n");
            break;
        }

        run(line); // Execute the line

        hadScanParseError = false;
    }
}

static void run(const char* source) {
    Scanner scanner;
    initScanner(&scanner, source);
    int tokenCount = 0;
    Token* tokens = scanTokens(&scanner, &tokenCount);

    // TODO: Check for scanner errors if scanTokens indicates them

    Parser parser;
    initParser(&parser, tokens, tokenCount);
    StmtList* statements = parse(&parser);

    // debugging
    if (statements == NULL) {
        // printf("Parser returned NULL (parse error or empty input).\n");
        freeTokens(tokens, tokenCount);
        return;
    }

    resolve(NULL, statements);
    // Stop if there was a syntax error during parsing.
    if (hadParserError(&parser)) {
        hadScanParseError = true;
        freeStmtList(statements);
        freeTokens(tokens, tokenCount);
        return;
    }

    if (hadRuntimeError()) {
        freeStmtList(statements);
        freeTokens(tokens, tokenCount);
        return;
    }

    interpretStatements(statements);

    // clean up
    freeStmtList(statements);
    freeTokens(tokens, tokenCount);
}
