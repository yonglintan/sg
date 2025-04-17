#include <stdlib.h>
#include <stdio.h>

#include "memory.h"

// central function for all dynamic memory management.
// note: oldSize still unused for now.
void* reallocate(void* pointer, size_t oldSize, size_t newSize) {

    if (newSize == 0) {
        // means we are freeing mem
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);

    // check if reallocation failed
    if (result == NULL) {
        // todo: better handling?
        fprintf(stderr, "Fatal: Memory allocation failed! (realloc returned NULL)\n");
        exit(1); // exit if memory cannot be allocated/reallocated
    }

    return result;
} 