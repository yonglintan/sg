#ifndef sg_memory_h
#define sg_memory_h

#include <stddef.h> // For size_t

// Macro to calculate new capacity
#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

// Macro to handle resizing arrays
#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) * (newCount))

// Macro to free an array
#define FREE_ARRAY(type, pointer, oldCount) \
    reallocate(pointer, 0)

// Main reallocation function (handles allocation, reallocation, freeing)
// oldSize = 0, newSize > 0  => Allocate new block
// oldSize > 0, newSize > 0  => Change allocation size
// oldSize > 0, newSize = 0  => Free allocation
void* reallocate(void* pointer, size_t newSize);

// Convenience macros using reallocate
#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, sizeof(type) * (count))

#define FREE(type, pointer) \
    reallocate(pointer, 0)

#define REALLOCATE(type, pointer, old_count, new_count) \
    (type*)reallocate(pointer, sizeof(type) * (new_count))

#endif