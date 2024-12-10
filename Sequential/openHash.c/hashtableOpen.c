#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define SIZE 15

struct Node {
    char* key;
    int value;
    int deleted;
};

struct HashTable {
    struct Node** table;
    int size;
};

pthread_mutex_t lock;

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
    pthread_mutex_lock(&lock);
    int index = hashFunction(key, hashtable->size);
    int originalIndex = index;
    int i = 0;

    while (hashtable->table[index] != NULL && hashtable->table[index]->deleted == 0) {
        // Prevent infinite looping
        if (hashtable->table[index]->key != NULL && strcmp(hashtable->table[index]->key, key) == 0) {
            printf("Key already exists: %s\n", key);
            pthread_mutex_unlock(&lock);
            return;
        }
        if (i >= hashtable->size) {
            printf("HashTable is full! Cannot insert %s.\n", key);
            pthread_mutex_unlock(&lock);
            return;
        }

        index = (index + 1) % hashtable->size;
        i++;
    }

    if (hashtable->table[index] == NULL || hashtable->table[index]->deleted == 1) {
        struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
        newNode->key = strdup(key);
        newNode->value = value;
        newNode->deleted = 0;
        hashtable->table[index] = newNode;
        printf("Inserted %s -> %d at index %d , had original index %d\n", key, value, index, originalIndex);
    }

    pthread_mutex_unlock(&lock);
}

struct Node* search(struct HashTable* hashtable, const char* key) {
    pthread_mutex_lock(&lock);

    int index = hashFunction(key, hashtable->size);
    int i = 0;

    while (i < hashtable->size) {
        if (hashtable->table[index] == NULL) {
            break; // Empty slot, key not present
        }

        if (hashtable->table[index]->deleted == 0 &&
            strcmp(hashtable->table[index]->key, key) == 0) {
            printf("Found: %s -> %d at index %d\n", key, hashtable->table[index]->value, index);
            pthread_mutex_unlock(&lock);
            return hashtable->table[index];
        }

        index = (index + 1) % hashtable->size;
        i++;
    }

    printf("Key not found: %s\n", key);
    pthread_mutex_unlock(&lock);
    return NULL;
}

void deleteNode(struct HashTable* hashtable, const char* key) {
    pthread_mutex_lock(&lock);

    int index = hashFunction(key, hashtable->size);
    int i = 0;

    while (i < hashtable->size) {
        if (hashtable->table[index] == NULL) {
            break; // Empty slot, key not present
        }

        if (hashtable->table[index]->deleted == 0 &&
            strcmp(hashtable->table[index]->key, key) == 0) {
            hashtable->table[index]->deleted = 1;
            printf("Key deleted: %s\n", key);
            pthread_mutex_unlock(&lock);
            return;
        }

        index = (index + 1) % hashtable->size;
        i++;
    }

    printf("Key not found: %s\n", key);
    pthread_mutex_unlock(&lock);
}

void destroyHashTable(struct HashTable* hashtable) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < hashtable->size; i++) {
        if (hashtable->table[i] != NULL) {
            free(hashtable->table[i]->key);
            free(hashtable->table[i]);
        }
    }
    free(hashtable->table);
    free(hashtable);
    pthread_mutex_unlock(&lock);
}

void* work(void* arg) {
    struct HashTable* hashtable = (struct HashTable*)arg;
    char names[10][10] = {"Alice", "Bob", "Charlie", "David", "Eve",
                          "Frank", "Grace", "Heidi", "Ivan", "Judy"};
    for (int i = 0; i < 10; i++) {
        insert(hashtable, names[i], i + 1);
    }
    return NULL;
}

int main() {
    pthread_mutex_init(&lock, NULL);

    struct HashTable* hashtable = createHashTable(SIZE);

    int nbrThreads = 2; // Number of threads
    pthread_t threads[nbrThreads];

    // Create threads
    for (int i = 0; i < nbrThreads; i++) {
        pthread_create(&threads[i], NULL, work, hashtable);
    }

    // Wait for threads to finish
    for (int i = 0; i < nbrThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Search for five names
    char* searchNames[5] = {"Alice", "Eve", "Ivan", "Charlie", "Grace"};
    for (int i = 0; i < 5; i++) {
        search(hashtable, searchNames[i]);
    }

    // Delete all names
    char names[10][10] = {"Alice", "Bob", "Charlie", "David", "Eve",
                          "Frank", "Grace", "Heidi", "Ivan", "Judy"};
    for (int i = 0; i < 10; i++) {
        deleteNode(hashtable, names[i]);
    }

    // Destroy hash table
    destroyHashTable(hashtable);

    pthread_mutex_destroy(&lock);

    return 0;
}
