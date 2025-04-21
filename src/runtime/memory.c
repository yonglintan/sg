#include <stdio.h>
#include <stdlib.h>

#include "memory.h"

// central function for all dynamic memory management.
// note: oldSize still unused for now.
void* reallocate(void* pointer, size_t newSize) {

    if (newSize == 0) {
        // means we are freeing mem
        free(pointer);
        return NULL;
    }

    void* result = realloc(pointer, newSize);

    // check if reallocation failed
    if (result == NULL) {
        // todo: better handling?
        fprintf(stderr, "Aiyo die already lah: Memory allocation fail leh! realloc return NULL sia...\n");
        exit(1); // exit if memory cannot be allocated/reallocated
    }

    return result;
}