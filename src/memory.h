#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

typedef struct {
    void *data;
    size_t bytes_used;
    size_t capacity;
} memory_arena;

memory_arena *create_arena(size_t capacity);

void *allocate_from_arena(memory_arena *arena, size_t size);
void delete_arena(memory_arena *arena);

#endif //MEMORY_H
