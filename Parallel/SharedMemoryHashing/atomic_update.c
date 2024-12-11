#include "atomic_update.h"

bool atomicUpdateOverwrite(MyElement *expected, const MyElement *desired, Overwrite f) {
    MyElement_CAS(expected, desired);
    return true;
}

bool atomicUpdateIncrement(MyElement *expected, const MyElement *desired, Increment f) {
    __sync_fetch_and_add(&expected->data, 1);
    return true;
}

bool atomicUpdateDecrement(MyElement *expected, const MyElement *desired, Decrement f) {
    __sync_fetch_and_sub(&expected->data, 1);
    return true;
}
