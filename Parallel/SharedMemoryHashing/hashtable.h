#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stddef.h>
#include <stdbool.h>
#include "my_element.h"
#include "atomic_update.h"

#define MAX_DIST 100

typedef struct {
    MyElement *table;
    size_t mask;
    size_t size;
} HashTable;

HashTable *HashTable_init(size_t logSize);
void HashTable_free(HashTable *ht);
bool HashTable_insert(HashTable *ht, const MyElement *e);
MyElement HashTable_find(HashTable *ht, long long key);
bool HashTable_insertOrUpdate(HashTable *ht, const MyElement *e, Overwrite f);
bool HashTable_insertOrUpdateAtomic(HashTable *ht, const MyElement *e, Increment f);

#endif // HASHTABLE_H
