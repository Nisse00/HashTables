#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

struct Node {
    char *key;
    int value;
    struct Node *next;
};

struct HashTable {
    int size;
    struct Node **table;
};

pthread_mutex_t lock;

struct Node *search(struct HashTable *hashTable, const char *key);

void createHashTable(struct HashTable *hashTable, int size) {
    hashTable->size = size;
    hashTable->table = malloc(size * sizeof(struct Node *));
    for (int i = 0; i < size; i++) {
        hashTable->table[i] = NULL;
    }
}

int hashFunction(const char *key, int size) {
    unsigned long hash = 5381; 
    int c;

    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c; 
    }
    return hash % size;
}

void insert(struct HashTable *hashTable, const char *key, int value) {
    pthread_mutex_lock(&lock);
    if (search(hashTable, key) != NULL) {
        printf("Key already exists: %s\n", key);
        pthread_mutex_unlock(&lock);
        return;
    }

    int index = hashFunction(key, hashTable->size);
    struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
    newNode->key = strdup(key); 
    newNode->value = value;
    newNode->next = hashTable->table[index]; 
    hashTable->table[index] = newNode; 

    printf("Inserted %s -> %d at index %d\n", key, value, index);

    pthread_mutex_unlock(&lock);
}

struct Node *search(struct HashTable *hashTable, const char *key) {
    pthread_mutex_lock(&lock);

    int index = hashFunction(key, hashTable->size);
    struct Node *current = hashTable->table[index];

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            printf("Found: %s -> %d\n", current->key, current->value);
            pthread_mutex_unlock(&lock);
            return current;
        }
        current = current->next;
    }

    printf("Key not found: %s\n", key);
    pthread_mutex_unlock(&lock);
    return NULL;
}

int delete(struct HashTable *hashTable, const char *key) {
    pthread_mutex_lock(&lock);

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

            printf("Key deleted: %s\n", key);
            pthread_mutex_unlock(&lock);
            return 1;
        }
        prev = current;
        current = current->next;
    }

    printf("Key not found: %s\n", key);
    pthread_mutex_unlock(&lock);
    return 0;
}

void destroyHashTable(struct HashTable *hashTable) {
    pthread_mutex_lock(&lock);

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

    pthread_mutex_unlock(&lock);
}

void *work(void *arg) {
    struct HashTable *hashTable = (struct HashTable *)arg;
    char names[10][10] = {"Alice", "Bob", "Charlie", "David", "Eve",
                          "Frank", "Grace", "Heidi", "Ivan", "Judy"};
    for (int i = 0; i < 10; i++) {
        insert(hashTable, names[i], i + 1);
    }
    return NULL;
}

int main() {
    //Have to use a recursive mutex because of needing to call serach() inside insert()
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&lock, &attr);
    pthread_mutexattr_destroy(&attr);

    struct HashTable hashTable;
    createHashTable(&hashTable, 100);

    int nbrThreads = 100; 
    pthread_t threads[nbrThreads];

    for (int i = 0; i < nbrThreads; i++) {
        pthread_create(&threads[i], NULL, work, &hashTable);
    }

    for (int i = 0; i < nbrThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    char *searchKeys[5] = {"Alice", "Eve", "Ivan", "Charlie", "Grace"};
    for (int i = 0; i < 5; i++) {
        search(&hashTable, searchKeys[i]);
    }

    char *deleteKeys[10] = {"Alice", "Bob", "Charlie", "David", "Eve",
                            "Frank", "Grace", "Heidi", "Ivan", "Judy"};
    for (int i = 0; i < 10; i++) {
        delete(&hashTable, deleteKeys[i]);
    }

    destroyHashTable(&hashTable);

    pthread_mutex_destroy(&lock);
    return 0;
}
