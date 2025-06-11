#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "memory.h"
#include "strings.h"
#include <stdbool.h>
#include <stdint.h>

enum ht_entry_status {
    EMPTY,
    TOMBSTONE,
    OCCUPIED,
};

typedef struct {
    const string *key;
    const void *value;
    enum ht_entry_status status;
} ht_entry;

typedef struct {
    size_t capacity;
    size_t length;
    ht_entry entries[];
} ht;

ht *ht_alloc(size_t max_entries, memory_arena *arena);
const void *ht_get(ht *hash_table, const string *key);
void ht_add(ht *hash_table, const void *entry, const string *key);
void ht_remove(ht *hash_table, const string *key);

#endif // HASH_TABLE_H
