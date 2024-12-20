#ifndef MYELEMENT_H
#define MYELEMENT_H

#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#define MAX_KEY_LENGTH 100

typedef struct {
    char key[MAX_KEY_LENGTH];  // String key
    long long data;            // Associated data (e.g., count)
} MyElement;


MyElement MyElement_init(const char *key, long long data);
MyElement MyElement_getEmptyValue();
bool MyElement_isEmpty(const MyElement *e);
bool MyElement_CAS(MyElement *expected, const MyElement *desired);

#endif // MYELEMENT_H
