//This is a implementation of a hash table using open addressing with linear probing.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 3


struct Node {
    char* key;
    int value;
    int deleted;
};

struct HashTable {
    struct Node** table;
    int size;
};

struct HashTable* createHashTable(int size) {
    struct HashTable* hashtable = (struct HashTable*)malloc(sizeof(struct HashTable));
    hashtable->size = size;
    hashtable->table = (struct Node**)malloc(size * sizeof(struct Node*));
    for (int i = 0; i < size; i++) {
        hashtable->table[i] = NULL;
    }
    return hashtable;
}

int hashFunction(const char* key, int size) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % size;
}

void insert(struct HashTable* hashtable, const char* key, int value) {
    int index = hashFunction(key, hashtable->size);
    printf("First index calculated: %d for %s\n", index, key);
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    newNode->key = strdup(key);
    newNode->value = value;
    if (hashtable->table[index] != NULL) {
        int i = 1;
        while (hashtable->table[(index + i) % hashtable->size] != NULL) {
            i++;
        }
        index = (index + i) % hashtable->size;
    } 
    printf("Inserted %s -> %d at index %d\n", key, value, index);
    hashtable->table[index] = newNode;
}

struct Node* search(struct HashTable* hashtable, const char* key) {
    int index = hashFunction(key, hashtable->size);
    int i = 0;
    while (hashtable->table[(index + i) % hashtable->size] != NULL) {
        if (strcmp(hashtable->table[(index + i) % hashtable->size]->key, key) == 0 && hashtable->table[(index + i) % hashtable->size]->deleted == 0) {
            printf("Found: %s -> %d at index %d\n", hashtable->table[(index + i) % hashtable->size]->key, hashtable->table[(index + i) % hashtable->size]->value, (index + i) % hashtable->size);
            return hashtable->table[(index + i) % hashtable->size];
        }
        i++;
    }
    printf("Key not found.\n");
    return NULL;
}

void deleteNode(struct HashTable* hashtable, const char* key) {
    int index = hashFunction(key, hashtable->size);
    int i = 0;
    while (hashtable->table[(index + i) % hashtable->size] != NULL) {
        if (strcmp(hashtable->table[(index + i) % hashtable->size]->key, key) == 0) {
            hashtable->table[(index + i) % hashtable->size]->deleted = 1;
            printf("Key deleted %s\n", key);
            return;
        }
        i++;
    }
    printf("Key not found.\n");
}

void destroyHashTable(struct HashTable* hashtable) {
    for (int i = 0; i < hashtable->size; i++) {
        if (hashtable->table[i] != NULL) {
            free(hashtable->table[i]->key);
            free(hashtable->table[i]);
        }
    }
    free(hashtable->table);
    free(hashtable);
}



int main() {
    struct HashTable* table = createHashTable(SIZE);
    insert(table, "Anna", 1); //Gets hash 1 and is put at index 1
    insert(table, "Andrea", 2); // Gets hash 1 and is put at index 2
    insert(table, "Hanna", 3); //Gets hash 1 and is put at index 0

    struct Node* node = search(table, "Anna");

    deleteNode(table, "Anna");

    node = search(table, "Andrea");

    destroyHashTable(table);
    return 0;
}