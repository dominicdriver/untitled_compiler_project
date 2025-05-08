#include "memory.h"

#include <stdlib.h>
#include <stdio.h>

#define MAX_ALIGNMENT 8

memory_arena *create_arena(size_t capacity) {
    memory_arena *arena = malloc(sizeof(memory_arena));
    arena->data = malloc(capacity);
    arena->bytes_used = 0;
    arena->capacity = capacity;

    return arena;
}

void *allocate_from_arena(memory_arena* arena, size_t size) {
    size_t aligned_size = size + (-size & (MAX_ALIGNMENT - 1));

    if (arena->bytes_used + aligned_size > arena->capacity) {
        printf("\033[91mOut of Memory! Goodbye!\033[0m\n");
        exit(1);
    }

    arena->bytes_used += aligned_size;

    return (char*) arena->data + arena->bytes_used - aligned_size;
}

void delete_arena(memory_arena *arena) {
    free(arena->data);
    free(arena);
}
