#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "scanner.h"
#include "expr.h"

// Global error flag
static bool hadError = false;

// Error reporting functions
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
    
    // Exit with error code if there was an error
    if (hadError) exit(65);
}

static void runPrompt(void) {
    char line[1024];
    
    for (;;) {
        printf("> ");
        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }
        
        // Remove newline character if present
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        run(line);
        
        // Reset error flag in REPL mode
        hadError = false;
    }
}

static void run(const char* source) {
    Scanner scanner;
    initScanner(&scanner, source);
    
    int tokenCount = 0;
    Token* tokens = scanTokens(&scanner, &tokenCount);

    // Print tokens and handle errors
    for (int i = 0; i < tokenCount; i++) {
        Token token = tokens[i];
        
        // Handle error tokens
        if (token.type == TOKEN_ERROR) {
            error(token.line, token.start);
            // Don't print error tokens
        } else {
            // Only print non-error tokens
            printToken(token);
        }
    }
    
    // Free tokens and allocated error messages
    freeTokens(tokens, tokenCount);
}
