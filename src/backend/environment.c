#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../frontend/scanner.h"
#include "../runtime/memory.h"
#include "../runtime/object.h"
#include "environment.h"

#define INITIAL_CAPACITY 8

// Helper to initialize the entry array within an environment
static void initEntries(Environment* environment, int capacity) {
    // printf("Initializing environment entries with capacity: %d\n", capacity);

    environment->entries = ALLOCATE(Entry, capacity);
    // Check allocation success
    if (environment->entries == NULL && capacity > 0) {
        fprintf(stderr, "Memory problem lah: Cannot allocate environment entries leh.\n");
        environment->capacity = 0; // Mark as invalid
        environment->count = 0;
        return;
    }
    environment->capacity = capacity;
    environment->count = 0;
    // initialize entry names to NULL for safe freeing
    for (int i = 0; i < capacity; i++) {
        // set to default values
        environment->entries[i].name = NULL;
        environment->entries[i].value = NIL_VAL;
    }
}

// initialize a new top-level (global) environment
Environment* newEnvironment() {
    Environment* environment = ALLOCATE(Environment, 1);
    if (environment == NULL) {
        fprintf(stderr, "Memory problem lah: Cannot allocate environment sia.\n");
        return NULL;
    }
    initEntries(environment, INITIAL_CAPACITY);
    if (environment->capacity == 0 && INITIAL_CAPACITY > 0) { // Check if initEntries failed
        FREE(Environment, environment);
        return NULL;
    }
    environment->enclosing = NULL;
    return environment;
}

// Initialize a new nested environment
Environment* newEnclosedEnvironment(Environment* enclosing) {
    Environment* environment = ALLOCATE(Environment, 1);
    if (environment == NULL) {
        fprintf(stderr, "Memory problem lah: Cannot allocate enclosed environment ok?\n");
        return NULL;
    }
    initEntries(environment, INITIAL_CAPACITY);
    if (environment->capacity == 0 && INITIAL_CAPACITY > 0) { // Check if initEntries failed
        FREE(Environment, environment);
        return NULL;
    }
    environment->enclosing = enclosing;
    return environment;
}

// Free an environment and its entries
void freeEnvironment(Environment* environment) {
    if (environment == NULL) return;
    for (int i = 0; i < environment->count; i++) {
        // free the copied variable name string
        if (environment->entries[i].name != NULL) {
            FREE(char, environment->entries[i].name);
        }
    }
    if (environment->capacity > 0) {
        FREE_ARRAY(Entry, environment->entries, environment->capacity);
    }

    FREE(Environment, environment);
}

static bool growCapacity(Environment* environment) {
    int oldCapacity = environment->capacity;

    // just grow by 2x for now, can improve later
    int newCapacity = oldCapacity < INITIAL_CAPACITY ? INITIAL_CAPACITY : oldCapacity * 2;
    Entry* newEntries = REALLOCATE(Entry, environment->entries, oldCapacity, newCapacity);

    if (newEntries == NULL) {
        fprintf(stderr, "Memory problem lah: Cannot reallocate environment entries one leh.\n");
        return false;
    }

    // initialize new slots
    for (int i = oldCapacity; i < newCapacity; i++) {
        newEntries[i].name = NULL;
        newEntries[i].value = NIL_VAL;
    }

    environment->entries = newEntries;
    environment->capacity = newCapacity;
    return true;
}

bool environmentDefine(Environment* environment, const char* name, Value value) {
    if (environment == NULL || name == NULL) return false;

    // Check if variable already exists in the current scope for redefinition
    for (int i = 0; i < environment->count; i++) {
        if (environment->entries[i].name != NULL && strcmp(environment->entries[i].name, name) == 0) {
            // Overwrite existing variable in this scope
            environment->entries[i].value = value;
            return true;
        }
    }

    // if not redefining, add a new entry. grow capacity if needed.
    if (environment->count >= environment->capacity) {
        if (!growCapacity(environment)) {
            printf("Failed to grow capacity for environment.\n");
            return false;
        }
    }

    // Add the new variable
    char* nameCopy = ALLOCATE(char, strlen(name) + 1);
    if (nameCopy == NULL) {
        fprintf(stderr, "Memory problem lah: Cannot allocate variable name copy ok?.\n");
        return false;
    }
    strcpy(nameCopy, name);

    environment->entries[environment->count].name = nameCopy;
    environment->entries[environment->count].value = value;
    environment->count++;
    return true;
}

static char* tokenToString(Token* token) {
    if (token == NULL || token->start == NULL) return NULL;
    char* lexeme = malloc(token->length + 1); // + 1 for null terminator
    if (lexeme == NULL) return NULL;
    strncpy(lexeme, token->start, token->length);
    lexeme[token->length] = '\0';
    return lexeme;
}

// get a variable's value. checks current scope then enclosing scopes.
bool environmentGet(Environment* environment, Token* nameToken, Value* outValue) {
    if (environment == NULL || nameToken == NULL || outValue == NULL) return false;

    char* name = tokenToString(nameToken);
    if (name == NULL) {
        fprintf(stderr, "Memory problem lah: Got issue when processing variable name token sia.\n");
        return false;
    }

    // check current scope
    for (int i = 0; i < environment->count; i++) {
        if (environment->entries[i].name != NULL && strcmp(environment->entries[i].name, name) == 0) {
            *outValue = environment->entries[i].value;
            free(name); // free the copied name string
            return true;
        }
    }

    // if not found, check enclosing scope (recursive)
    if (environment->enclosing != NULL) {
        bool found = environmentGet(environment->enclosing, nameToken, outValue);
        free(name); // free the name allocated in this call regardless of find result
        return found;
    }

    // printf("Variable '%s' not found in any scope.\n", name);
    free(name);
    return false;
}

// assign a value to an *existing* variable. checks current scope then enclosing.
bool environmentAssign(Environment* environment, Token* nameToken, Value value) {
    // NOTE: if we decide to support CONSTANTS, can check here?

    if (environment == NULL || nameToken == NULL) return false;

    char* name = tokenToString(nameToken);
    if (name == NULL) {
        fprintf(stderr, "Memory problem lah: Processing variable name token for assignment also got problem leh.\n");
        return false;
    }

    // check current scope first
    for (int i = 0; i < environment->count; i++) {
        if (environment->entries[i].name != NULL && strcmp(environment->entries[i].name, name) == 0) {
            environment->entries[i].value = value;
            free(name); // free the copied name string
            return true;
        }
    }

    // if not found, try assigning in enclosing scope (recursive)
    if (environment->enclosing != NULL) {
        bool assigned = environmentAssign(environment->enclosing, nameToken, value);
        free(name); // free the name allocated in this call regardless of assignment result
        return assigned;
    }

    // variable not found in any scope for assignment
    printf("Variable '%s' not found in any scope for assignment.\n", name);
    free(name);
    return false;
}