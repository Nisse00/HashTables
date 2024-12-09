//This is a implementation of a closed hash table with chaining.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    char *key;             
    int value;              
    struct Node *next;      
};

struct HashTable {
    int size;              
    struct Node **table;   
};


void createHashTable(struct HashTable *hashTable, int size) {
    hashTable->size = size;
    hashTable->table = malloc(size * sizeof(struct Node *));
    for (int i = 0; i < size; i++) {
        hashTable->table[i] = NULL;
    }
}

int hashFunction(const char *key, int size) {
    unsigned long hash = 5381; // A common hash seed
    int c;

    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    printf("Index: %lu\n", hash % size);
    return hash % size;
}

void insert(struct HashTable *hashTable, const char *key, int value) {
    int index = hashFunction(key, hashTable->size);

    struct Node *newNode = (struct Node *) malloc(sizeof(struct Node));
    newNode->key = strdup(key); // Allocate memory and copy the key
    newNode->value = value;
    newNode->next = hashTable->table[index]; // We make sure that the new node points to the current head of the list

    hashTable->table[index] = newNode; // The new node is now the head of the list
}

struct Node *search(struct HashTable *hashTable, const char *key) {
    int index = hashFunction(key, hashTable->size);
    struct Node *current = hashTable->table[index];

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) { // Check if the key matches
            printf("Found: %s -> %d\n", current->key, current->value);
            return current; 
        }
        current = current->next;
    }
    printf("Key not found.\n");
    return NULL; 
}

int delete(struct HashTable *hashTable, const char *key) {
    int index = hashFunction(key, hashTable->size);
    struct Node *current = hashTable->table[index];
    struct Node *prev = NULL;

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            if (prev == NULL) {
                hashTable->table[index] = current->next;
            } else {
                prev->next = current->next;
            }

            free(current->key);
            free(current);

            return 1; 
        }
        prev = current;
        current = current->next;
    }

    return 0;
}



void destroyHashTable(struct HashTable *hashTable) {
    for (int i = 0; i < hashTable->size; i++) {
        struct Node *current = hashTable->table[i];
        while (current != NULL) {
            struct Node *temp = current;
            current = current->next;
            free(temp->key); 
            free(temp);  
        }
    }
    free(hashTable->table);
}

int main() {
    struct HashTable hashTable;
    createHashTable(&hashTable, 5);

    insert(&hashTable, "Alice", 25);
    insert(&hashTable, "Bob", 30);
    //insert(&hashTable, "Charlie", 35);

    struct Node *node = search(&hashTable, "Alice");
    struct Node *node2 = search(&hashTable, "Bob");

    printf("Bob next: %d\n" , node2->next->value); //This should print Alice's value since Bob and alice have the same index
    destroyHashTable(&hashTable);
    return 0;
}
