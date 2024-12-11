#ifndef ATOMICUPDATE_H
#define ATOMICUPDATE_H

#include "my_element.h"

typedef struct {
    int dummy; // Empty structure used as a type
} Overwrite;

typedef struct {
    int dummy; // Empty structure used as a type
} Increment;

typedef struct {
    int dummy; // Empty structure used as a type
} Decrement;

bool atomicUpdateOverwrite(MyElement *expected, const MyElement *desired, Overwrite f);
bool atomicUpdateIncrement(MyElement *expected, const MyElement *desired, Increment f);
bool atomicUpdateDecrement(MyElement *expected, const MyElement *desired, Decrement f);

#endif // ATOMICUPDATE_H
