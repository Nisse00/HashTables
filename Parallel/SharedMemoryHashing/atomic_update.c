#include "atomic_update.h"
#include <stdbool.h>  // For bool
#include <string.h>   // For strcmp

bool atomicUpdateOverwrite(MyElement *expected, const MyElement *desired, Overwrite f) {
    // Try to atomically set the desired value
    if (MyElement_CAS(expected, desired)) {
        return true;  // Successfully inserted
    } else if (strcmp(expected->key, desired->key) == 0) {
        // If CAS failed but the keys match, increment the value
        return atomicUpdateIncrement(expected, desired, (Increment){});
    } else {
        // Keys don’t match; insertion failed
        return false;
    }
}

bool atomicUpdateIncrement(MyElement *expected, const MyElement *desired, Increment f) {
    while (true) {
        // Capture the current value of data
        long long oldData = expected->data;

        // Attempt to increment atomically
        if (__sync_bool_compare_and_swap(&expected->data, oldData, oldData + 1)) {
            return true;  // Increment succeeded
        }
    }
}

bool atomicUpdateDecrement(MyElement *expected, const MyElement *desired, Decrement f) {
    while (true) {
        // Capture the current value of data
        long long oldData = expected->data;

        // Attempt to decrement atomically
        if (__sync_bool_compare_and_swap(&expected->data, oldData, oldData - 1)) {
            return true;  // Decrement succeeded
        }
    }
}
