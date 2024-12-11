#ifndef MYELEMENT_H
#define MYELEMENT_H

#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

typedef struct {
    long long key;
    long long data;
} MyElement;

MyElement MyElement_init(long long key, long long data);
MyElement MyElement_getEmptyValue();
bool MyElement_isEmpty(const MyElement *e);
bool MyElement_CAS(MyElement *expected, const MyElement *desired);
void MyElement_update(MyElement *e, const MyElement *new_data);
void MyElement_copy(MyElement *dest, const MyElement *src);

#endif // MYELEMENT_H
