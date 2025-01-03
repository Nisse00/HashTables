#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define HASH_TABLE_SIZE 16777216 // Larger size for ~10 million words
#define MAX_WORD_LENGTH 50       // Maximum word length
#define NUM_THREADS 1  // Number of threads

// Node structure for open addressing
struct Node {
    char* key;
    int value;
    int deleted;
};

// Hash table structure
struct HashTable {
    struct Node** table;
    int size;
};

pthread_mutex_t lock;

// Function declarations
struct HashTable* createHashTable(int size);
void insert(struct HashTable* hashtable, const char* key, int value);
struct Node* search(struct HashTable* hashtable, const char* key);
void destroyHashTable(struct HashTable* hashtable);

// Hash function
int hashFunction(const char* key, int size) {
    unsigned long hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % size;
}

// Create the hash table
struct HashTable* createHashTable(int size) {
    struct HashTable* hashtable = (struct HashTable*)malloc(sizeof(struct HashTable));
    hashtable->size = size;
    hashtable->table = (struct Node**)calloc(size, sizeof(struct Node*));
    return hashtable;
}

// Insert into the hash table (open addressing)
void insert(struct HashTable* hashtable, const char* key, int value) {
    int index = hashFunction(key, hashtable->size);
    int originalIndex = index;

    for (int i = 0; i < hashtable->size; i++) {
        pthread_mutex_lock(&lock);

        if (hashtable->table[index] == NULL || hashtable->table[index]->deleted) {
            struct Node* newNode = malloc(sizeof(struct Node));
            newNode->key = strdup(key);
            newNode->value = value;
            newNode->deleted = 0;
            hashtable->table[index] = newNode;
            pthread_mutex_unlock(&lock);
            return;
        }

        if (strcmp(hashtable->table[index]->key, key) == 0) {
            hashtable->table[index]->value++;
            pthread_mutex_unlock(&lock);
            return;
        }

        pthread_mutex_unlock(&lock);
        index = (originalIndex + i) % hashtable->size; // Linear probing
    }
}

// Destroy the hash table
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

// Thread data structure
struct ThreadData {
    char** words;
    int wordCount;
    struct HashTable* hashtable;
};

// Thread function
void* processWords(void* arg) {
    struct ThreadData* data = (struct ThreadData*)arg;

    for (int i = 0; i < data->wordCount; i++) {
        insert(data->hashtable, data->words[i], 1);
    }
    return NULL;
}

int main() {
    pthread_mutex_init(&lock, NULL);

    // Measure time
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Step 1: Read the file 10 times
    FILE* file;
    char** words = malloc(10000000 * sizeof(char*));
    if (!words) {
        perror("Failed to allocate memory for words");
        return 1;
    }

    int totalWords = 0;
    char line[4096];

    for (int pass = 0; pass < 10; pass++) {
        file = fopen("Lorem-ipsum-dolor-sit-amet.txt", "r");
        if (!file) {
            perror("Could not open file");
            free(words);
            return 1;
        }

        while (fgets(line, sizeof(line), file) && totalWords < 10000000) {
            char* token = strtok(line, " ,.-\n");
            while (token != NULL && totalWords < 10000000) {
                words[totalWords] = strdup(token);
                totalWords++;
                token = strtok(NULL, " ,.-\n");
            }
        }

        fclose(file);
        printf("Pass %d completed, total words so far: %d\n", pass + 1, totalWords);
    }

    printf("Total words read: %d\n", totalWords);

    // Step 2: Create the hash table
    struct HashTable* hashtable = createHashTable(HASH_TABLE_SIZE);

    // Step 3: Split words among threads
    pthread_t threads[NUM_THREADS];
    struct ThreadData threadData[NUM_THREADS];

    int wordsPerThread = totalWords / NUM_THREADS;
    int remainder = totalWords % NUM_THREADS;

    int currentWord = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        threadData[i].words = &words[currentWord];
        threadData[i].wordCount = wordsPerThread;
        threadData[i].hashtable = hashtable;

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

    // Step 5: Cleanup
    for (int i = 0; i < totalWords; i++) {
        free(words[i]);
    }
    free(words);
    destroyHashTable(hashtable);
    pthread_mutex_destroy(&lock);

    // Measure time
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Program execution time: %.6f seconds\n", elapsed);

    return 0;
}
