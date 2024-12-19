#include "hashtable.h"
#include "my_element.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NUM_THREADS 10
#define MAX_WORDS 10000000// Total words to handle
#define FILE_READS 10     // Number of times to read the file
#define MAX_WORD_LENGTH 100

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
        MyElement e = MyElement_init(i, strlen(tArgs->words[i]));
        if (!HashTable_insert(tArgs->ht, &e)) {
            printf("Thread %ld: Failed to insert key %d\n", pthread_self(), i);
        }
    }
    return NULL;
}

int main() {
    // Measure time
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Initialize the hash table
    size_t logSize = 28; // 2^12 = 4096 slots in the hash table
    HashTable *ht = HashTable_init(logSize);
    printf("HashTable initialized with %d slots\n", 1 << logSize);

    // Allocate memory to store words
    char **words = malloc(MAX_WORDS * sizeof(char *));
    if (!words) {
        perror("Failed to allocate memory for words");
        return EXIT_FAILURE;
    }

    // Step 1: Read the file 10 times
    const char *filePath = "/Users/nils/Programmering/projektDatavetenskap/Lorem-ipsum-dolor-sit-amet.txt";
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
        printf("Pass %d completed, total words so far: %d\n", pass + 1, totalWords);
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

        printf("Main thread: Creating thread %d to process words %d to %d\n", i, args[i].start, args[i].end);
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
    printf("Cleaning up hash table\n");
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
