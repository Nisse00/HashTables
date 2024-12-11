#include "hashtable.h"
#include "my_element.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 20
#define NUM_ELEMENTS 1000

// Structure to pass arguments to threads
typedef struct {
    HashTable *ht;
    int start;
    int end;
} ThreadArgs;

// Thread function for parallel inserts
void *threadInsert(void *args) {
    ThreadArgs *tArgs = (ThreadArgs *)args;
    //printf("Thread %ld started, processing elements from %d to %d\n", pthread_self(), tArgs->start, tArgs->end);
    
    for (int i = tArgs->start; i < tArgs->end; ++i) {
        MyElement e = MyElement_init(i, i * 10);
        //printf("Thread %ld: Inserting key %d with data %lld\n", pthread_self(), i, e.data);
        if (!HashTable_insert(tArgs->ht, &e)) {
            printf("Thread %ld: Failed to insert key %d\n", pthread_self(), i);
        } else {
            printf("Thread %ld: Successfully inserted key %d\n", pthread_self(), i);
        }
    }
    
    printf("Thread %ld finished\n", pthread_self());
    return NULL;
}

int main() {
    // Initialize the hash table
    size_t logSize = 12; // 2^12 = 4096 slots in the hash table
    HashTable *ht = HashTable_init(logSize);
    printf("HashTable initialized with %d slots\n", 1 << logSize);

    // Create threads and distribute work
    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];
    int elementsPerThread = NUM_ELEMENTS / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; ++i) {
        args[i].ht = ht;
        args[i].start = i * elementsPerThread;
        args[i].end = (i + 1) * elementsPerThread;
        
        printf("Main thread: Creating thread %d to handle elements from %d to %d\n", i, args[i].start, args[i].end);
        if (pthread_create(&threads[i], NULL, threadInsert, &args[i]) != 0) {
            printf("Failed to create thread\n");
            return EXIT_FAILURE;
        }
    }

    // Join threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        if (pthread_join(threads[i], NULL) != 0) {
            printf("Failed to join thread %d\n", i);
            return EXIT_FAILURE;
        }
    }

    // Verify insertion
    printf("Verifying insertion...\n");
    for (int i = 0; i < NUM_ELEMENTS; ++i) {
        MyElement found = HashTable_find(ht, i);
        if (!MyElement_isEmpty(&found)) {
            printf("Found Key: Key %d -> Data %lld\n", i, found.data);
        } else {
            printf("Key %d not found!\n", i);
        }
    }

    // Clean up
    printf("Cleaning up hash table\n");
    HashTable_free(ht);
    return 0;
}
