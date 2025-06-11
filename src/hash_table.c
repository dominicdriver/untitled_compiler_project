#include "hash_table.h"
#include "helper_functions.h"
#include "memory.h"
#include "strings.h"

#include <assert.h>
#include <string.h>

// Hash Table using open addressing with linear probing

ht *ht_alloc(size_t max_entries, memory_arena *arena) {
    ht *hash_table = allocate_from_arena(arena, sizeof(ht) + max_entries * sizeof(ht_entry));

    memset(hash_table, 0, sizeof(ht) + max_entries * sizeof(ht_entry));

    hash_table->capacity = max_entries;
    hash_table->length = 0;

    return hash_table;
}

const void *ht_get(ht *hash_table, const string *key) {
    assert(hash_table != NULL);
    assert(key != NULL);

    size_t index = hash(key) % hash_table->capacity;
    ht_entry *entry = &hash_table->entries[index];

    while (entry->status != EMPTY) {
        if (entry->status == OCCUPIED && (string_cmp(key, entry->key) == 0)) {
            return entry->value;
        }

        entry++;

        if ((size_t) (entry - hash_table->entries) == hash_table->capacity) {
            entry = hash_table->entries;
        }
    }

    return NULL;
}

void ht_add(ht *hash_table, const void *entry, const string *key) {
    assert(hash_table != NULL);
    assert(entry != NULL);
    assert(key != NULL);

    assert(hash_table->length < hash_table->capacity);

    size_t index = hash(key) % hash_table->capacity;
    size_t tombstone_index = 0;
    bool tombstone_found = false;

    while (hash_table->entries[index].status != EMPTY) {
        if (hash_table->entries[index].status == TOMBSTONE && !tombstone_found) {
            tombstone_index = index;
        }

        index++;
        index %= hash_table->capacity;
    }

    if (tombstone_found) {
        hash_table->entries[tombstone_index].key = key;
        hash_table->entries[tombstone_index].value = entry;
        hash_table->entries[tombstone_index].status = OCCUPIED;
    } else {
        hash_table->entries[index].key = key;
        hash_table->entries[index].value = entry;
        hash_table->entries[index].status = OCCUPIED;
    }

    hash_table->length++;
}

void ht_remove(ht *hash_table, const string *key) {
    assert(hash_table != NULL);
    assert(key != NULL);

    size_t index = hash(key) % hash_table->capacity;
    size_t start_index = index;

    while (hash_table->entries[index].status != EMPTY) {
        if (hash_table->entries[index].status == OCCUPIED && (string_cmp(key, hash_table->entries[index].key) == 0)) {
            hash_table->entries[index].status = TOMBSTONE;

            hash_table->length--;
        }

        index++;
        index %= hash_table->capacity;

        if (index == start_index) {
            break;
        }
    }
}
