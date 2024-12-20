#include "hashtable.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>  // For error printing
#include "atomic_update.h"

// If LONG_LONG_MAX is not available, define it manually
#ifndef LONG_LONG_MAX
#define LONG_LONG_MAX 9223372036854775807LL
#endif

static size_t hash(const char *str, size_t mask) {
    size_t hash = 5381;  // Start with a large prime
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    }

    return hash & mask;  // Apply the mask to fit within table size
}


HashTable *HashTable_init(size_t logSize) {
    HashTable *ht = (HashTable *)malloc(sizeof(HashTable));
    if (!ht) {
        fprintf(stderr, "Memory allocation failed for HashTable.\n");
        return NULL;  // Return NULL instead of exiting
    }

    ht->size = (1ULL << logSize) - 1;
    ht->mask = ht->size;
    ht->table = (MyElement *)aligned_alloc(16, (ht->size + 1) * sizeof(MyElement));  // Use aligned_alloc
    if (!ht->table) {
        free(ht);
        fprintf(stderr, "Memory allocation failed for HashTable table.\n");
        return NULL;  // Return NULL instead of exiting
    }

    for (size_t i = 0; i <= ht->size; i++) {
        ht->table[i] = MyElement_getEmptyValue();
    }
    return ht;
}


void HashTable_free(HashTable *ht) {
    free(ht->table); 
    free(ht);
}


MyElement HashTable_find(HashTable *ht, const char *key) {
    size_t h = hash(key, ht->mask);  // Use the updated hash function
    for (size_t i = h; i < h + MAX_DIST; ++i) {
        MyElement *current = &ht->table[i & ht->mask];

        if (strcmp(current->key, key) == 0) {  // Compare strings
            return *current;  // Found the key
        }

        if (MyElement_isEmpty(current)) {
            break;  // Empty slot means the key isn't present
        }
    }
    return MyElement_getEmptyValue();  // Return empty if not found
}

bool HashTable_insertOrUpdateIncrement(HashTable *ht, const MyElement *e, Increment f) {
    size_t h = hash(e->key, ht->mask);

    // Iteratively retry until success or table is full
    for (size_t i = h; i < h + MAX_DIST; ++i) {
        MyElement *current = &ht->table[i & ht->mask];

        // If the key matches, increment the count atomically
        if (strcmp(current->key, e->key) == 0) {
            return atomicUpdateIncrement(current, e, f);
        }

        // If the slot is empty, try to insert atomically
        if (MyElement_isEmpty(current)) {
            if (MyElement_CAS(current, e)) {
                return true;  // Successfully inserted
            } else {
                // CAS failed; retry the current index
                i--;  // Decrement `i` to retry this slot
                continue;
            }
        }
    }

    return false;  // Table is full or max probing distance exceeded
}



bool HashTable_insertOrUpdateDecrement(HashTable *ht, const MyElement *e, Decrement f) {
    size_t h = hash(e->key, ht->mask);
    for (size_t i = h; i < h + MAX_DIST; ++i) {
        MyElement *current = &ht->table[i & ht->mask];

        if (strcmp(current->key, e->key) == 0) {
            // Increment the count atomically
            atomicUpdateDecrement(current, e, f);
            return true;
        }

        if (MyElement_isEmpty(current)) {
            // Try to atomically insert using CAS
            if (MyElement_CAS(current, e)) {
                return true;
            }
        }
    }
    return false;  // Table is full or max probing distance exceeded
}