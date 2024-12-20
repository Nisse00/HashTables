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
MyElement HashTable_find(HashTable *ht, const char *key);
bool HashTable_insertOrUpdateIncrement(HashTable *ht, const MyElement *e, Increment f);
bool hashTable_insertOrUpdateDecrement(HashTable *ht, const MyElement *e, Decrement f);

#endif // HASHTABLE_H
