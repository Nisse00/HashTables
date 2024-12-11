#include "hashtable.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>  // For error printing
#include "atomic_update.h"

// If LONG_LONG_MAX is not available, define it manually
#ifndef LONG_LONG_MAX
#define LONG_LONG_MAX 9223372036854775807LL
#endif

static size_t hash(long long key, size_t mask) {
    return key % mask;
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

// Use free instead of _aligned_free
void HashTable_free(HashTable *ht) {
    free(ht->table);  // Use free instead of _aligned_free
    free(ht);
}

bool HashTable_insert(HashTable *ht, const MyElement *e) {
    size_t h = hash(e->key, ht->mask);
    Overwrite overwriteOp;

    for (size_t i = h; i < h + MAX_DIST; ++i) {
        MyElement *current = &ht->table[i & ht->mask];
        // If the slot is empty, try to atomically insert
        if (MyElement_isEmpty(current)) {
            if (atomicUpdateOverwrite(current, e, overwriteOp)) {
                return true;
            }
        }

        // If the key already exists, we cannot insert
        if (current->key == e->key) {
            return false;
        }
    }
    return false; // Could not find a place for the element
}

MyElement HashTable_find(HashTable *ht, long long key) {
    size_t h = hash(key, ht->mask);
    for (size_t i = h; i < h + MAX_DIST; ++i) {
        MyElement *current = &ht->table[i & ht->mask];
        // Check if the current slot is empty or contains the key
        if (current->key == key) {
            //printf("Found it!, the key is %lld with data %lld\n", current->key, current->data);
            return *current;
        }
        if (current->key == LONG_LONG_MAX) {
            //printf("Slot is empty\n");
            break;  
        }
    }
    return MyElement_getEmptyValue();  // Return an empty element if not found
}


bool HashTable_insertOrUpdate(HashTable *ht, const MyElement *e, Overwrite f) {
    size_t h = hash(e->key, ht->mask);
    for (size_t i = h; i < h + MAX_DIST; ++i) {
        MyElement *current = &ht->table[i & ht->mask];
        if (current->key == e->key) {
            atomicUpdateOverwrite(current, e, f);
            return true;
        }
        if (MyElement_isEmpty(current)) {
            MyElement_copy(current, e);
            return true;
        }
    }
    return false;
}

bool HashTable_insertOrUpdateAtomic(HashTable *ht, const MyElement *e, Increment f) {
    size_t h = hash(e->key, ht->mask);
    for (size_t i = h; i < h + MAX_DIST; ++i) {
        MyElement *current = &ht->table[i & ht->mask];
        if (current->key == e->key) {
            atomicUpdateIncrement(current, e, f);
            return true;
        }
        if (MyElement_isEmpty(current)) {
            MyElement_copy(current, e);
            return true;
        }
    }
    return false;
}

bool HashTable_insertOrUpdateDecrement(HashTable *ht, const MyElement *e, Decrement f) {
    size_t h = hash(e->key, ht->mask);
    for (size_t i = h; i < h + MAX_DIST; ++i) {
        MyElement *current = &ht->table[i & ht->mask];
        if (current->key == e->key) {
            atomicUpdateDecrement(current, e, f);  // Decrement operation
            return true;
        }
        if (MyElement_isEmpty(current)) {
            MyElement_copy(current, e);
            return true;
        }
    }
    return false;
}

