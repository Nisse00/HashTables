#include "my_element.h"
#include <limits.h>  // For LONG_LONG_MAX, if it's available
#include <stdio.h>   // For printf
#include <string.h>  // For strncpy

// If LONG_LONG_MAX is not available, define it manually
#ifndef LONG_LONG_MAX
#define LONG_LONG_MAX 9223372036854775807LL
#endif

MyElement MyElement_init(const char *key, long long data) {
    MyElement e;
    strncpy(e.key, key, MAX_KEY_LENGTH - 1);  // Copy key string
    e.key[MAX_KEY_LENGTH - 1] = '\0';         // Ensure null-terminated
    e.data = data;
    return e;
}

MyElement MyElement_getEmptyValue() {
    return MyElement_init("", 0);  // Empty key and data
}

bool MyElement_isEmpty(const MyElement *e) {
    return e->key[0] == '\0';  // Empty if key is an empty string
}

bool MyElement_CAS(MyElement *expected, const MyElement *desired) {
    while (true) {
        // Capture the current value of data
        long long oldData = expected->data;

        // Attempt the CAS operation
        if (__sync_bool_compare_and_swap(&expected->data, oldData, desired->data)) {
            // If successful, copy the string key atomically
            strncpy(expected->key, desired->key, MAX_KEY_LENGTH - 1);
            expected->key[MAX_KEY_LENGTH - 1] = '\0';  // Ensure null-terminated
            return true;
        }

        // Check if the value has been updated by another thread
        if (expected->data != oldData) {
            // Another thread has already updated the value; exit the loop
            return false;
        }

        // Retry the CAS operation
    }
}
