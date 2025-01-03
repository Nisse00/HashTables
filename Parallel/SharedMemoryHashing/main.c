#include "hashtable.h"
#include "my_element.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NUM_THREADS 32
#define MAX_WORDS 10000000 // Total words to handle
#define FILE_READS 10      // Number of times to read the file
#define MAX_WORD_LENGTH 50

// Structure to pass arguments to threads
typedef struct {
    HashTable *ht;
    char **words;
    int start;
    int end;
} ThreadArgs;

// Thread function for parallel inserts
void *threadInsert(void *args) {
    ThreadArgs *tArgs = (ThreadArgs *)args;

    for (int i = tArgs->start; i < tArgs->end; ++i) {
        MyElement e = MyElement_init(tArgs->words[i], 1);  // Use string as key
        if (!HashTable_insertOrUpdateIncrement(tArgs->ht, &e, (Increment){})) {
            printf("Failed to insert key \"%s\"\n", tArgs->words[i]);
        }
    }
    return NULL;
}

int main() {
    // Measure time
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Initialize the hash table
    size_t logSize = 24; // 2^24 slots in the hash table (16M slots)
    HashTable *ht = HashTable_init(logSize);
    printf("HashTable initialized with %d slots\n", 1 << logSize);

    // Allocate memory to store words
    char **words = malloc(MAX_WORDS * sizeof(char *));
    if (!words) {
        perror("Failed to allocate memory for words");
        return EXIT_FAILURE;
    }

    // Step 1: Read the file 10 times
    const char *filePath = "Lorem-ipsum-dolor-sit-amet.txt";
    char line[4096];
    int totalWords = 0;

    for (int pass = 0; pass < FILE_READS; ++pass) {
        FILE *file = fopen(filePath, "r");
        if (!file) {
            perror("Could not open file");
            free(words);
            return EXIT_FAILURE;
        }

        while (fgets(line, sizeof(line), file)) {
            char *token = strtok(line, " ,.-\n");
            while (token != NULL) {
                if (totalWords < MAX_WORDS) {
                    words[totalWords] = strdup(token);
                    totalWords++;
                }
                token = strtok(NULL, " ,.-\n");
            }
        }
        fclose(file);
    }


    printf("Total words read: %d\n", totalWords);

    // Step 2: Split work among threads
    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];
    int wordsPerThread = totalWords / NUM_THREADS;
    int remainder = totalWords % NUM_THREADS;

    int currentWord = 0;
    for (int i = 0; i < NUM_THREADS; ++i) {
        args[i].ht = ht;
        args[i].words = words;
        args[i].start = currentWord;
        args[i].end = currentWord + wordsPerThread;

        if (remainder > 0) {
            args[i].end++;
            remainder--;
        }
        currentWord = args[i].end;

        if (pthread_create(&threads[i], NULL, threadInsert, &args[i]) != 0) {
            perror("Failed to create thread");
            free(words);
            return EXIT_FAILURE;
        }
    }

    // Step 3: Join threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        if (pthread_join(threads[i], NULL) != 0) {
            printf("Failed to join thread %d\n", i);
            free(words);
            return EXIT_FAILURE;
        }
    }

    // Step 4: Cleanup
    for (int i = 0; i < totalWords; ++i) {
        free(words[i]);
    }
    free(words);
    HashTable_free(ht);

    // Measure time
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("Program execution time: %.6f seconds\n", elapsed);

    return 0;
}

