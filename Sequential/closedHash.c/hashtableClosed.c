#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define HASH_TABLE_SIZE 16777216 // 2^24
#define MAX_WORD_LENGTH 50      // Maximum word length
#define NUM_THREADS 16    // Number of threads

// Node structure for hash table
struct Node {
    char *key;
    int value;
    struct Node *next;
};

// Hash table structure
struct HashTable {
    int size;
    struct Node **table;
};

// Thread data structure
struct ThreadData {
    char **words;
    int wordCount;
};

// Global variables
struct HashTable hashTable;
pthread_mutex_t lock;

// Function declarations
void createHashTable(struct HashTable *hashTable, int size);
void insert(struct HashTable *hashTable, const char *key);
struct Node *find(struct HashTable *hashTable, const char *key);
void destroyHashTable(struct HashTable *hashTable);

// Hash function
int hashFunction(const char *key, int size) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % size;
}

// Create the hash table
void createHashTable(struct HashTable *hashTable, int size) {
    hashTable->size = size;
    hashTable->table = calloc(size, sizeof(struct Node *));
}

// Insert into the hash table
void insert(struct HashTable *hashTable, const char *key) {
    pthread_mutex_lock(&lock);
    int index = hashFunction(key, hashTable->size);

    // Check if the word already exists
    struct Node *current = hashTable->table[index];
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            current->value++;
            pthread_mutex_unlock(&lock);
            return;
        }
        current = current->next;
    }

    // Add a new node
    struct Node *newNode = malloc(sizeof(struct Node));
    newNode->key = strdup(key);
    newNode->value = 1;
    newNode->next = hashTable->table[index];
    hashTable->table[index] = newNode;

    pthread_mutex_unlock(&lock);
}

// Destroy the hash table
void destroyHashTable(struct HashTable *hashTable) {
    for (int i = 0; i < hashTable->size; i++) {
        struct Node *current = hashTable->table[i];
        while (current) {
            struct Node *temp = current;
            current = current->next;
            free(temp->key);
            free(temp);
        }
    }
    free(hashTable->table);
}

// Thread function
void *processWords(void *arg) {
    struct ThreadData *data = (struct ThreadData *)arg;

    for (int i = 0; i < data->wordCount; i++) {
        insert(&hashTable, data->words[i]);
    }
    return NULL;
}

struct Node *find(struct HashTable *hashTable, const char *key) {
    int index = hashFunction(key, hashTable->size); // Get the hash index
    struct Node *current = hashTable->table[index];

    // Traverse the linked list at the given index
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            return current; // Return the node if the key matches
        }
        current = current->next;
    }
    return NULL; // Key not found
}


int main() {
    pthread_mutex_init(&lock, NULL);

    // Measure time
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

// Step 1: Read the file 10 times
FILE *file;
char **words = malloc(10000000 * sizeof(char *)); // Allocate space for 10 million words
if (!words) {
    perror("Failed to allocate memory for words");
    return 1;
}

int totalWords = 0;
char line[4096];

// Read the file 10 times
for (int pass = 0; pass < 10; pass++) {
    file = fopen("Lorem-ipsum-dolor-sit-amet.txt", "r");
    if (!file) {
        perror("Could not open file");
        free(words);
        return 1;
    }

    // Read the file line by line and tokenize
    while (fgets(line, sizeof(line), file) && totalWords < 10000000) {
        char *token = strtok(line, " ,.-\n"); // Tokenize words
        while (token != NULL && totalWords < 10000000) {
            words[totalWords] = strdup(token); // Duplicate the word
            if (words[totalWords] == NULL) {  // Check for memory allocation failure
                perror("Failed to allocate memory for a word");
                fclose(file);
                for (int i = 0; i < totalWords; i++) {
                    free(words[i]);
                }
                free(words);
                return 1;
            }
            totalWords++;
            token = strtok(NULL, " ,.-\n");
        }
    }

    fclose(file); // Close file after each pass
    printf("Pass %d completed, total words so far: %d\n", pass + 1, totalWords);
}

printf("Total words read: %d\n", totalWords);


    // Step 2: Initialize hash table
    createHashTable(&hashTable, HASH_TABLE_SIZE);

    // Step 3: Split words among threads
    pthread_t threads[NUM_THREADS];
    struct ThreadData threadData[NUM_THREADS];

    int wordsPerThread = totalWords / NUM_THREADS;
    int remainder = totalWords % NUM_THREADS;

    int currentWord = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        threadData[i].words = &words[currentWord];
        threadData[i].wordCount = wordsPerThread;

        if (remainder > 0) {
            threadData[i].wordCount++;
            remainder--;
        }
        currentWord += threadData[i].wordCount;
    }

    // Step 4: Create and join threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, processWords, &threadData[i]);
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Step 5: Find a word
    /* struct Node *result = find(&hashTable, "Lorem");
    if (result) {
        printf("Word 'Lorem' appears %d times.\n", result->value);
    } else {
        printf("Word 'Lorem' not found.\n");
    }
    */

    // Step 6: Cleanup
    for (int i = 0; i < totalWords; i++) {
        free(words[i]);
    }
    free(words);
    destroyHashTable(&hashTable);
    pthread_mutex_destroy(&lock);

    // Measure time
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Program execution time: %.6f seconds\n", elapsed);

    return 0;
}
