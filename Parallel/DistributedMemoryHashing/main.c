#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h> 

#define HASH_TABLE_SIZE 10000  // Increased table size
#define NUM_PES 20                // Number of Processing Elements
#define MAX_STRING_LENGTH 100      // Maximum string length
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

// Hash function to map a string to a hash table index
int hash(const char* str) {
    unsigned long hash = 5381;
    for (int i = 0; str[i] != '\0'; i++) {
        hash = ((hash << 5) + hash) + str[i]; // hash * 33 + str[i]
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

        // If the slot is empty, insert the new string
        if (ht->table[probeIdx].key[0] == '\0') {
            strcpy(ht->table[probeIdx].key, key);
            ht->table[probeIdx].value = 1;
            return;
        }

        // If the string already exists, increment its count
        if (strcmp(ht->table[probeIdx].key, key) == 0) {
            ht->table[probeIdx].value++;
            return;
        }
    }
    printf("Error: Hash table is full. Cannot insert %s\n", key);
}

// Global find function to query all PEs
int globalFind(const char* key) {
    int idx = hash(key) % HASH_TABLE_SIZE;
    int targetPE = responsiblePE(idx);

    pthread_mutex_lock(&hashTables[targetPE].lock);
    int result = 0;
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        int probeIdx = (idx + i) % HASH_TABLE_SIZE;
        if (strcmp(hashTables[targetPE].table[probeIdx].key, key) == 0) {
            result = hashTables[targetPE].table[probeIdx].value;
            break;
        }
    }
    pthread_mutex_unlock(&hashTables[targetPE].lock);
    return result;
}

// Process a list of strings for a given PE
void* processList(void* arg) {
    int peId = *(int*)arg;

    // Process strings in the list
    for (int i = 0; i < stringCounts[peId]; i++) {
        char* str = stringLists[peId][i];
        int idx = hash(str) % HASH_TABLE_SIZE;
        int targetPE = responsiblePE(idx);

        if (targetPE == peId) {
            pthread_mutex_lock(&hashTables[peId].lock);
            update(&hashTables[peId], str, idx);
            pthread_mutex_unlock(&hashTables[peId].lock);
        } else {
            pthread_mutex_lock(&hashTables[targetPE].lock);
            update(&hashTables[targetPE], str, idx);
            pthread_mutex_unlock(&hashTables[targetPE].lock);
        }
    }
    return NULL;
}

int main() {
    pthread_t threads[NUM_PES];
    int peIds[NUM_PES];

    // Measure time
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Initialize hash tables
    for (int i = 0; i < NUM_PES; i++) {
        memset(hashTables[i].table, 0, sizeof(hashTables[i].table));
        pthread_mutex_init(&hashTables[i].lock, NULL);
        stringCounts[i] = 0; // Reset counts for each PE
    }

    // Step 1: Read the file 10 times and distribute words across PEs
    const char* filePath = "/Users/nils/Programmering/projektDatavetenskap/Lorem-ipsum-dolor-sit-amet.txt";
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
                // Assign to PEs in round-robin fashion
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
        printf("Pass %d completed, total words so far: %d\n", pass + 1, totalWords);
    }

    printf("Total words read: %d\n", totalWords);

    // Step 2: Process lists in parallel
    for (int i = 0; i < NUM_PES; i++) {
        peIds[i] = i;
        pthread_create(&threads[i], NULL, processList, &peIds[i]);
    }

    for (int i = 0; i < NUM_PES; i++) {
        pthread_join(threads[i], NULL);
    }

    // Step 3: Find some strings
    const char* queryStrings[] = {"Lorem", "ipsum", "dolor", "sit", "amet", "unknown"};
    for (int i = 0; i < sizeof(queryStrings) / sizeof(queryStrings[0]); i++) {
        int count = globalFind(queryStrings[i]);
        printf("Find \"%s\": %d\n", queryStrings[i], count);
    }

    // Step 4: Cleanup
    for (int i = 0; i < NUM_PES; i++) {
        pthread_mutex_destroy(&hashTables[i].lock);
    }

    // Measure time
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Program execution time: %.6f seconds\n", elapsed);

    return 0;
}

