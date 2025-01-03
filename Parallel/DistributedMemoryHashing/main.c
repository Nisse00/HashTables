#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define GLOBAL_HASH_TABLE_SIZE 16777216
#define HASH_TABLE_SIZE (GLOBAL_HASH_TABLE_SIZE / NUM_PES)
#define BATCH_SIZE 10000000 / NUM_PES
#define MAX_STRING_LENGTH 50
#define MAX_STRINGS_PER_PE (10000000 / NUM_PES)
#define NUM_PES 16 // Fixed number of partitions

typedef struct {
    char key[MAX_STRING_LENGTH];
    int value;
    struct HashEntry* next;
} HashEntry;

typedef struct {
    HashEntry** table;
    int size;
    pthread_mutex_t lock;
} HashTable;

typedef struct {
    char key[MAX_STRING_LENGTH];
    int value;
} Operation;

typedef struct {
    Operation* operations; // Array of operations
    int count;
} Batch;

HashTable hashTables[NUM_PES];
Batch localBatches[NUM_PES];
pthread_mutex_t PELocks[NUM_PES];
char*** stringLists;
int* stringCounts;

// Hash function
int hash(const char* str, int size) {
    unsigned long hash = 5381;
    for (int i = 0; str[i] != '\0'; i++) {
        hash = ((hash << 5) + hash) + str[i];
    }
    return abs((int)(hash % size));
}

// Responsible Partition
int responsiblePE(int index) {
    return index % NUM_PES;
}

// Insert into hash table
void hashTableInsert(HashTable* ht, const char* key, int value) {
    int idx = hash(key, ht->size);
    HashEntry* current = ht->table[idx];

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            current->value += value;
            return;
        }
        current = current->next;
    }

    HashEntry* newEntry = malloc(sizeof(HashEntry));
    strncpy(newEntry->key, key, MAX_STRING_LENGTH);
    newEntry->key[MAX_STRING_LENGTH - 1] = '\0';
    newEntry->value = value;
    newEntry->next = ht->table[idx];
    ht->table[idx] = newEntry;
}

// Find in hash table
int hashTableFind(HashTable* ht, const char* key) {
    if (!ht || !ht->table) return 0;

    int idx = hash(key, ht->size);
    HashEntry* current = ht->table[idx];
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) return current->value;
        current = current->next;
    }

    return 0;
}

// Allocate memory
void allocateMemory() {
    stringLists = malloc(NUM_PES * sizeof(char**));
    stringCounts = malloc(NUM_PES * sizeof(int));

    for (int i = 0; i < NUM_PES; i++) {
        hashTables[i].table = calloc(HASH_TABLE_SIZE, sizeof(HashEntry*));
        hashTables[i].size = HASH_TABLE_SIZE;
        pthread_mutex_init(&PELocks[i], NULL);

        localBatches[i].operations = malloc(BATCH_SIZE * sizeof(Operation));
        localBatches[i].count = 0;

        stringLists[i] = malloc(MAX_STRINGS_PER_PE * sizeof(char*));
        for (int j = 0; j < MAX_STRINGS_PER_PE; j++) {
            stringLists[i][j] = malloc((MAX_STRING_LENGTH + 1) * sizeof(char));
        }
        stringCounts[i] = 0;
    }
}

// Free memory
void freeMemory() {
    for (int i = 0; i < NUM_PES; i++) {
        for (int j = 0; j < MAX_STRINGS_PER_PE; j++) {
            free(stringLists[i][j]);
        }
        free(stringLists[i]);
        free(hashTables[i].table);
        free(localBatches[i].operations);
        pthread_mutex_destroy(&PELocks[i]);
    }
    free(stringLists);
    free(stringCounts);
}

// Add operation to batch
void addOperationToBatch(int partition, const char* key, int value) { // Add operation to batch
    pthread_mutex_lock(&PELocks[partition]); // Lock the partition
    if (localBatches[partition].count < BATCH_SIZE) { // If the batch is not full
        Operation* op = &localBatches[partition].operations[localBatches[partition].count++]; 
        strncpy(op->key, key, MAX_STRING_LENGTH);
        op->key[MAX_STRING_LENGTH - 1] = '\0';
        op->value = value;
    }
    pthread_mutex_unlock(&PELocks[partition]);
}

// Process Batch
void* processBatch(void* arg) {
    int PE = *(int*)arg;

    pthread_mutex_lock(&PELocks[PE]);
    for (int i = 0; i < localBatches[PE].count; i++) {
        Operation* op = &localBatches[PE].operations[i];
        hashTableInsert(&hashTables[PE], op->key, op->value);
    }
    localBatches[PE].count = 0;
    pthread_mutex_unlock(&PELocks[PE]);

    return NULL;
}

// Main function
int main() {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    pthread_t threads[NUM_PES];
    int PEids[NUM_PES];

    allocateMemory();

    const char* filePath = "Lorem-ipsum-dolor-sit-amet.txt";
    char line[4096];
    int totalWords = 0;

    for (int pass = 0; pass < 10; pass++) {  // Read the file 10 times
        FILE* file = fopen(filePath, "r");
        if (!file) {
            perror("Could not open file");
            freeMemory();
            return 1;
        }

        while (fgets(line, sizeof(line), file)) {
            char* token = strtok(line, " ,.-\n");
            while (token != NULL && totalWords < 10000000) {
                int partition = responsiblePE(hash(token, GLOBAL_HASH_TABLE_SIZE)); // Determine partition
                addOperationToBatch(partition, token, 1); // Add operation to batch of the partition
                totalWords++;
                token = strtok(NULL, " ,.-\n");
            }
        }
        fclose(file);

        // Process partitions in parallel
        for (int i = 0; i < NUM_PES; i++) {
            PEids[i] = i;
            pthread_create(&threads[i], NULL, processBatch, &PEids[i]);
        }

        for (int i = 0; i < NUM_PES; i++) {
            pthread_join(threads[i], NULL);
        }

        // Reset string counts
        for (int i = 0; i < NUM_PES; i++) {
            stringCounts[i] = 0;
        }

       // printf("Completed pass %d, total words: %d\n", pass + 1, totalWords);
    }
    /*
    const char* keysToCheck[] = {"Lorem", "ipsum", "dolor", "sit", "amet"};
    for (int i = 0; i < 5; i++) {
        const char* key = keysToCheck[i];
        int partition = responsiblePE(hash(key, GLOBAL_HASH_TABLE_SIZE));
        int value = hashTableFind(&hashTables[partition], key);

        printf("Key: %s, Value: %d (Stored in Partition %d)\n", key, value, partition);
    }
    */

    freeMemory();
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Program execution time: %.6f seconds\n", elapsed);
    return 0;
}
