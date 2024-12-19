#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define HASH_TABLE_SIZE 10000  // Increased table size
#define NUM_PES 15        // Number of Processing Elements
#define MAX_STRING_LENGTH 100  // Maximum string length
#define MAX_STRINGS_PER_PE 1000000 // Max strings a PE can handle

typedef struct {
    char key[MAX_STRING_LENGTH];
    int value;
} HashEntry;

typedef struct {
    HashEntry table[HASH_TABLE_SIZE];
    pthread_mutex_t lock;
} HashTable;

HashTable hashTables[NUM_PES];
char stringLists[NUM_PES][MAX_STRINGS_PER_PE][MAX_STRING_LENGTH];
int stringCounts[NUM_PES] = {0};

int localBuckets[NUM_PES][HASH_TABLE_SIZE] = {0}; // Local bucket counts
char localKeys[NUM_PES][HASH_TABLE_SIZE][MAX_STRING_LENGTH] = {{{'\0'}}}; // Local bucket keys
int receivedBuckets[NUM_PES][HASH_TABLE_SIZE] = {0}; // Received bucket counts
char receivedKeys[NUM_PES][HASH_TABLE_SIZE][MAX_STRING_LENGTH] = {{{'\0'}}}; // Received bucket keys
pthread_mutex_t bucketLocks[NUM_PES]; // Locks for bucket exchange

// Hash function to map a string to a hash table index
int hash(const char* str) {
    unsigned long hash = 5381;
    for (int i = 0; str[i] != '\0'; i++) {
        hash = ((hash << 5) + hash) + str[i];
    }
    return hash % HASH_TABLE_SIZE;
}

// Determine which PE is responsible for a given index
int responsiblePE(int index) {
    int rangeSize = HASH_TABLE_SIZE / NUM_PES;
    return index / rangeSize;
}

// Insert or update a string in the hash table
void update(HashTable* ht, const char* key, int idx) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        int probeIdx = (idx + i) % HASH_TABLE_SIZE;
        if (ht->table[probeIdx].key[0] == '\0') {
            strcpy(ht->table[probeIdx].key, key);
            ht->table[probeIdx].value = 1;
            return;
        }
        if (strcmp(ht->table[probeIdx].key, key) == 0) {
            ht->table[probeIdx].value++;
            return;
        }
    }
    printf("Error: Hash table is full. Cannot insert %s\n", key);
}

// Fill local buckets for each thread
void* processList(void* arg) {
    int peId = *(int*)arg;

    // Step 1: Fill local buckets
    for (int i = 0; i < stringCounts[peId]; i++) {
        char* str = stringLists[peId][i];
        int idx = hash(str) % HASH_TABLE_SIZE;
        int targetPE = responsiblePE(idx);

        int found = 0;
        for (int j = 0; j < HASH_TABLE_SIZE; j++) {
            if (strcmp(localKeys[targetPE][j], str) == 0) {
                localBuckets[targetPE][j]++;
                found = 1;
                break;
            } else if (localKeys[targetPE][j][0] == '\0') {
                strcpy(localKeys[targetPE][j], str);
                localBuckets[targetPE][j] = 1;
                found = 1;
                break;
            }
        }
        if (!found) {
            printf("Error: Local bucket for PE %d is full.\n", targetPE);
        }
    }

    // Step 2: Exchange buckets with responsible threads
    for (int targetPE = 0; targetPE < NUM_PES; targetPE++) {
        if (peId != targetPE) {
            pthread_mutex_lock(&bucketLocks[targetPE]);
            for (int j = 0; j < HASH_TABLE_SIZE; j++) {
                if (localKeys[targetPE][j][0] != '\0') {
                    strcpy(receivedKeys[targetPE][j], localKeys[targetPE][j]);
                    receivedBuckets[targetPE][j] = localBuckets[targetPE][j];
                }
            }
            pthread_mutex_unlock(&bucketLocks[targetPE]);
        }
    }

    // Step 3: Handle own bucket
    for (int j = 0; j < HASH_TABLE_SIZE; j++) {
        if (localKeys[peId][j][0] != '\0') {
            update(&hashTables[peId], localKeys[peId][j], hash(localKeys[peId][j]));
            hashTables[peId].table[hash(localKeys[peId][j])].value += localBuckets[peId][j] - 1;
        }
    }

    // Step 4: Process received buckets
    pthread_mutex_lock(&hashTables[peId].lock);
    for (int sourcePE = 0; sourcePE < NUM_PES; sourcePE++) {
        for (int j = 0; j < HASH_TABLE_SIZE; j++) {
            if (receivedKeys[sourcePE][j][0] != '\0') {
                update(&hashTables[peId], receivedKeys[sourcePE][j], hash(receivedKeys[sourcePE][j]));
                hashTables[peId].table[hash(receivedKeys[sourcePE][j])].value += receivedBuckets[sourcePE][j] - 1;
            }
        }
    }
    pthread_mutex_unlock(&hashTables[peId].lock);

    return NULL;
}


int main() {
    pthread_t threads[NUM_PES];
    int peIds[NUM_PES];

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Initialize hash tables and locks
    for (int i = 0; i < NUM_PES; i++) {
        memset(hashTables[i].table, 0, sizeof(hashTables[i].table));
        pthread_mutex_init(&hashTables[i].lock, NULL);
        pthread_mutex_init(&bucketLocks[i], NULL);
        stringCounts[i] = 0;
    }

    // Read file and distribute strings to PEs
    const char* filePath = "/Users/nils/Programmering/projektDatavetenskap/Lorem-ipsum-dolor-sit-amet.txt"; // Update with your file path
    char line[4096];
    int totalWords = 0;

    for (int pass = 0; pass < 10; pass++) {
        FILE* file = fopen(filePath, "r");
        if (!file) {
            perror("Could not open file");
            return 1;
        }
        while (fgets(line, sizeof(line), file)) {
            char* token = strtok(line, " ,.-\n");
            while (token != NULL) {
                int peId = totalWords % NUM_PES;
                if (stringCounts[peId] < MAX_STRINGS_PER_PE) {
                    strcpy(stringLists[peId][stringCounts[peId]++], token);
                } else {
                    printf("Warning: PE %d's string list is full.\n", peId);
                }
                totalWords++;
                token = strtok(NULL, " ,.-\n");
            }
        }
        fclose(file);
    }

    // Create threads to process lists
    for (int i = 0; i < NUM_PES; i++) {
        peIds[i] = i;
        pthread_create(&threads[i], NULL, processList, &peIds[i]);
    }

    // Wait for threads to finish
    for (int i = 0; i < NUM_PES; i++) {
        pthread_join(threads[i], NULL);
    }

    // Destroy locks
    for (int i = 0; i < NUM_PES; i++) {
        pthread_mutex_destroy(&hashTables[i].lock);
        pthread_mutex_destroy(&bucketLocks[i]);
    }

    // Measure execution time
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Program execution time: %.6f seconds\n", elapsed);

    return 0;
}

