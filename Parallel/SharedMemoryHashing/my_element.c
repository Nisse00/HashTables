#include "my_element.h"
#include <limits.h>  // For LONG_LONG_MAX, if it's available
#include <stdio.h>  // For printf

// If LONG_LONG_MAX is not available, define it manually
#ifndef LONG_LONG_MAX
#define LONG_LONG_MAX 9223372036854775807LL
#endif

MyElement MyElement_init(long long key, long long data) {
    MyElement e;
    e.key = key;
    e.data = data;
    return e;
}

MyElement MyElement_getEmptyValue() {
    return MyElement_init(LONG_LONG_MAX, 0);
}

bool MyElement_isEmpty(const MyElement *e) {
    return e->key == LONG_LONG_MAX;
}

bool MyElement_CAS(MyElement *expected, const MyElement *desired) {
    if (__sync_bool_compare_and_swap(&expected->data, expected->data, desired->data)) {
        expected->key = desired->key;
        return true;
    }
    return false;
}

void MyElement_update(MyElement *e, const MyElement *new_data) {
    e->data = new_data->data;
}

void MyElement_copy(MyElement *dest, const MyElement *src) {
    dest->key = src->key;
    dest->data = src->data;
}
