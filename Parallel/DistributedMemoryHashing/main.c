#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>

#define HASH_TABLE_SIZE 8
#define NUM_PES 3
#define MAX_WORD_LENGTH 100
#define MAX_WORDS_PER_PE 10

typedef struct {
    char key;
    int value;
} HashEntry;

typedef struct {
    HashEntry table[HASH_TABLE_SIZE];
    pthread_mutex_t lock;
} HashTable;

HashTable hashTables[NUM_PES];
char operations[NUM_PES][MAX_WORDS_PER_PE][MAX_WORD_LENGTH];
int opCounts[NUM_PES] = {0};

// Simple hash function to map keys to indices
int hash(char key) {
    return tolower(key) - 'a';
}

// Determine which PE is responsible for a given key
int responsiblePE(char key) {
    int range = 26 / NUM_PES;
    return hash(key) / range;
}

// Insert or update a key in the hash table
void update(HashTable* ht, char key, int value) {
    int idx = hash(key) % HASH_TABLE_SIZE;

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        int probeIdx = (idx + i) % HASH_TABLE_SIZE;
        if (ht->table[probeIdx].key == 0 || ht->table[probeIdx].key == key) {
            ht->table[probeIdx].key = key;
            ht->table[probeIdx].value += value;
            return;
        }
    }
}

// Process a batch of words for a given PE
void* processBatch(void* arg) {
    int peId = *(int*)arg;
    int localCounts[26] = {0}; // Local character counts for this PE
    int redistribution[NUM_PES][26] = {0}; // Redistribution buckets for other PEs

    // Process assigned words
    for (int i = 0; i < opCounts[peId]; i++) {
        char* word = operations[peId][i];
        for (int j = 0; word[j] != '\0'; j++) {
            char key = tolower(word[j]);
            int targetPE = responsiblePE(key);

            if (targetPE == peId) {
                // Count locally
                localCounts[hash(key)]++;
            } else {
                // Add to redistribution bucket
                redistribution[targetPE][hash(key)]++;
            }
        }
    }

    // Send redistributed counts to target PEs
    for (int target = 0; target < NUM_PES; target++) {
        if (target != peId) {
            pthread_mutex_lock(&hashTables[target].lock);
            for (int c = 0; c < 26; c++) {
                if (redistribution[target][c] > 0) {
                    char key = 'a' + c;
                    update(&hashTables[target], key, redistribution[target][c]);
                }
            }
            pthread_mutex_unlock(&hashTables[target].lock);
        }
    }

    // Update local hash table with localCounts
    pthread_mutex_lock(&hashTables[peId].lock);
    for (int c = 0; c < 26; c++) {
        if (localCounts[c] > 0) {
            char key = 'a' + c;
            update(&hashTables[peId], key, localCounts[c]);
        }
    }
    pthread_mutex_unlock(&hashTables[peId].lock);

    return NULL;
}

// Print the hash table for a given PE
void printHashTable(HashTable* ht, int peId) {
    printf("PE %d Hash Table:\n", peId);
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        if (ht->table[i].key != 0) {
            printf("  %c: %d\n", ht->table[i].key, ht->table[i].value);
        }
    }
    printf("\n");
}

int main() {
    pthread_t threads[NUM_PES];
    int peIds[NUM_PES];

    // Initialize hash tables
    for (int i = 0; i < NUM_PES; i++) {
        memset(hashTables[i].table, 0, sizeof(hashTables[i].table));
        pthread_mutex_init(&hashTables[i].lock, NULL);
    }

    // Example: assign words to PEs
    const char* words[] = {"tobeornot", "tobethatis", "thequestion"};
    int numWords = sizeof(words) / sizeof(words[0]);

    for (int i = 0; i < numWords; i++) {
        int peId = i % NUM_PES;
        strcpy(operations[peId][opCounts[peId]++], words[i]);
    }

    // Process batches in parallel
    for (int i = 0; i < NUM_PES; i++) {
        peIds[i] = i;
        pthread_create(&threads[i], NULL, processBatch, &peIds[i]);
    }

    for (int i = 0; i < NUM_PES; i++) {
        pthread_join(threads[i], NULL);
    }

    // Print hash tables
    for (int i = 0; i < NUM_PES; i++) {
        printHashTable(&hashTables[i], i);
    }

    // Cleanup
    for (int i = 0; i < NUM_PES; i++) {
        pthread_mutex_destroy(&hashTables[i].lock);
    }

    return 0;
}