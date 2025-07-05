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
    bool (*comp_func)(const string *s1, const string *s2);
    ht_entry entries[];
} ht;

ht *ht_alloc(size_t max_entries, bool comparision_function(const string *s1, const string *s2), memory_arena *arena);
const void *ht_get(ht *hash_table, const string *key);
void ht_add(ht *hash_table, const void *entry, const string *key);
void ht_remove(ht *hash_table, const string *key);

bool ht_compare_strcmp(const string *s1, const string *s2);

#endif // HASH_TABLE_H
